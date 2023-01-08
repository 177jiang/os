#include <constant.h>
#include <arch/x86/interrupts.h>
#include <hal/cpu.h>
#include <mm/mm.h>
#include <process.h>
#include <sched.h>
#include <status.h>

extern void _intr_print_msg(const char *fmt, ...);

void intr_routine_page_fault(const isr_param *param){

    uintptr_t pg_fault_ptr = cpu_rcr2();

    do{

        if(!pg_fault_ptr) break;

        struct mm_region *region = get_region(__current, pg_fault_ptr);
        if(!region) break;

        x86_pte_t *pte = __CURRENT_PTE(pg_fault_ptr);
        if(!*pte) break;

        if(*pte & PG_PRESENT){
            if((region->attr & REGION_PERM_MASK) !=
               (REGION_RSHARED | REGION_READ)){
                break;
            }

            uintptr_t page = vmm_dup_page(__current->pid, PG_ENTRY_ADDR(*pte));

            pmm_free_page(__current->pid, PG_ENTRY_ADDR(*pte));

            *pte = (*pte & 0xFFF) | page | PG_WRITE;
            return;
        }

        uintptr_t cache = *pte & ~0xFFF;
        if((region->attr & REGION_WRITE) &&
           (*pte & 0xFFF) && (!cache) ){
            uintptr_t page = pmm_alloc_page(__current->pid, 0);
            *pte = *pte | page | PG_PRESENT;
            return;
        }

    }while(0);

    printf_error("(pid: %d) Segmentation fault on %p (%p:%p)\n", 
                 __current->pid, pg_fault_ptr, param->cs, param->eip);

    terminate_task(SEGFAULT);

}
