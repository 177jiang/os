#include <constant.h>
#include <arch/x86/interrupts.h>
#include <hal/cpu.h>
#include <mm/mm.h>
#include <process.h>
#include <sched.h>
#include <status.h>

#define COW_MASK (REGION_RSHARED | REGION_READ | REGION_WRITE)

extern void _intr_print_msg(const char *fmt, ...);

int __do_kernel_pg_fault(v_mapping *mapping);

void intr_routine_page_fault(const isr_param *param){

    uintptr_t pg_fault_ptr = cpu_rcr2();


    do{

        if(!pg_fault_ptr) break;

        v_mapping mapping;
        if(!vmm_lookup(pg_fault_ptr, &mapping)){
            break;
        }

        /* kernel page fault */
        if(!SEL_RPL(param->cs)){
            if(__do_kernel_pg_fault(&mapping)){
                cpu_invplg(pg_fault_ptr);
                return ;
            }
        }

        struct mm_region *region = get_region(__current, pg_fault_ptr);

        if(!region) break;

        x86_pte_t *pte = __MOUNTED_PTE(PD_REFERENCED, pg_fault_ptr);

        if(!(*pte)) break;

        if((*pte & PG_PRESENT)){
            if((region->attr & COW_MASK) != COW_MASK){
                break;
            }
            cpu_invplg(pte);
            uintptr_t page = vmm_dup_page(__current->pid, PG_ENTRY_ADDR(*pte));

            pmm_free_page(__current->pid, PG_ENTRY_ADDR(*pte));

            *pte = PTE((*pte & 0xFFF)|PG_WRITE, page);

            return;

        }


        uintptr_t cache = *pte & ~0xFFF;
        if((region->attr & REGION_WRITE) &&
           (*pte & 0xFFF) && (!cache) ){
            cpu_invplg(pte);
            cpu_invplg(pg_fault_ptr);
            uintptr_t page = pmm_alloc_page(__current->pid, 0);

            *pte = PTE((*pte&0xFFF)|PG_PRESENT, page);

            return;
        }

    }while(0);

    printf_error("(pid: %d) Segmentation fault on %x (%p:%p)\n", 
                 __current->pid, pg_fault_ptr, param->cs, param->eip);

    __SIGSET(__current->sig_pending, _SIGSEGV);

    schedule();

}

int __do_kernel_pg_fault(v_mapping *mapping){

    uint32_t va = mapping->va;
    if(va >= K_HEAP_START && va < TASK_TABLE_START){
        uint32_t pa   = pmm_alloc_page(KERNEL_PID, 0);
        *mapping->pte = PTE(PG_PRESENT | (*mapping->pte&0xFFF), (pa));
        cpu_invplg(mapping->pte);
        cpu_invplg(va);
        return 1;
    }

    return 0;

}







