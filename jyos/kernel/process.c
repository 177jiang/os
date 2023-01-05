#include <process.h>
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <clock.h>
#include <libc/stdio.h>
#include <libc/string.h>

void dup_proc(){

    pid_t pid                 = alloc_pid();
    void       *new_dir_paddr = pmm_alloc_page(pid, PP_FGPERSIST);
    x86_page_t *new_dir_vaddr = vmm_map_page_occupy(pid, PG_MOUNT_1, new_dir_paddr, PG_PREM_RW);
    x86_page_t *page_dir      = PD_BASE_VADDR;

    for(uint32_t i=0; i<PD_LAST_INDEX; ++i){

        x86_pte_t pde = page_dir->entry[i];
        if(!pde || !(pde & PG_PRESENT)){
            new_dir_vaddr->entry[i] = pde;
            continue;
        }

        void       *new_pt_paddr = pmm_alloc_page(pid, PP_FGPERSIST);
        x86_page_t *new_pt_vaddr = vmm_map_page_occupy(pid, PG_MOUNT_2, new_pt_paddr, PG_PREM_RW);
        x86_page_t *page_table   = PT_VADDR(i);

        for(uint32_t j=0; j<PG_MAX_ENTRIES; ++j){

            x86_pte_t pte = page_table->entry[j];
            if(!pte || !(pte & PG_PRESENT)){
                new_pt_vaddr->entry[j] = pte;
                continue;
            }
            void *va = ((i << 10) | j) << 12;
            if(va >= K_STACK_START){
                void* pad = vmm_dup_page(va);
                pte       = PTE((pte & 0xFFF), pad);
            }
            new_pt_vaddr->entry[j] = pte;
        }

        new_dir_vaddr->entry[i] = PDE(PG_PREM_RW, new_pt_paddr);
    }
    new_dir_vaddr->entry[PD_LAST_INDEX] = PDE(PG_PREM_RW, new_dir_paddr);

    struct task_struct  new_task = {
        .pid            = pid,
        .page_table     = new_dir_paddr,
        .created        = clock_systime(),
        .regs           = __current->regs,
        .mm             = __current->mm,
        .parent_created = __current->created
    };

    new_task.regs.eax   = 0;
    __current->regs.eax = pid;
    push_task(&new_task);
}
