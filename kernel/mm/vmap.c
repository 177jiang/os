
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <constant.h>
#include <spike.h>

static uintptr_t _vmap_start = VMAP_START;

void * vmm_vmap(uintptr_t paddr, size_t n, page_attr attr){

    uintptr_t cur_addr    =  _vmap_start;
    unsigned int cur_size =  0;

    x86_page_t *page_dir  = PD_BASE_VADDR;

    while(cur_addr < VMAP_END){

        unsigned int pd_index = PD_INDEX(cur_addr);

        if(!page_dir->entry[pd_index]){

            cur_size += MEM_4MB;
            cur_addr = (cur_addr & 0xFFC00000) + MEM_4MB;

        }else{

            x86_page_t *page_table = PT_VADDR(pd_index);

            unsigned int pt_index = PT_INDEX(cur_addr);

            for(; pt_index<1024 && cur_size<n; ++pt_index){

                if(!page_table->entry[pt_index]){

                    cur_size += PG_SIZE;

                }else if(cur_size){

                    cur_size = 0;

                    ++pt_index;

                    break;

                }
            }

            cur_addr += (pt_index << 12);
        }

        if(cur_size >= n){
            goto __good_vmap;
        }

    }

    panick("VMM : out of memory\n");

__good_vmap:

    uintptr_t begin = cur_addr - cur_size;

    for(size_t i=0; i<n; i+=PG_SIZE){

        vmm_set_mapping(
            PD_REFERENCED,
            begin + i,
            paddr + i,
            attr,
            0
        );
        pmm_ref_page(KERNEL_PID, paddr + i);

    }

    _vmap_start = begin + n;

    return (void*)begin;

}
