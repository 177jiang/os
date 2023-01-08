#include <process.h>
#include <syscall.h>
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <clock.h>
#include <hal/cpu.h>
#include <libc/stdio.h>
#include <libc/string.h>


Pysical(void *) __dup_page_table(pid_t pid, uint32_t mount_point){

    /* moint_point is a pg_addr in dir*/

    void       *new_dir_paddr = pmm_alloc_page(pid, PP_FGPERSIST);
    x86_page_t *new_dir_vaddr = vmm_map_page_occupy(pid, PG_MOUNT_1, new_dir_paddr, PG_PREM_RW);
    x86_page_t *page_dir      = (x86_page_t *)((mount_point) | (0x3FF << 12));


    for(uint32_t i=0; i<PD_LAST_INDEX; ++i){

        x86_pte_t pde = page_dir->entry[i];
        if(!pde || !(pde & PG_PRESENT)){
            new_dir_vaddr->entry[i] = pde;
            continue;
        }

        void       *new_pt_paddr = pmm_alloc_page(pid, PP_FGPERSIST);
        x86_page_t *new_pt_vaddr = vmm_map_page_occupy(pid, PG_MOUNT_2, new_pt_paddr, PG_PREM_RW);

        x86_page_t *page_table   = (x86_page_t *)(mount_point | (i << 12));

        for(uint32_t j=0; j<PG_MAX_ENTRIES; ++j){

            uint32_t va = ((i << 10) | j ) << 12;

            x86_pte_t pte          = page_table->entry[j];
            pmm_ref_page(pid, PG_ENTRY_ADDR(pte));
            new_pt_vaddr->entry[j] = pte;
        }

        new_dir_vaddr->entry[i] = PDE(PG_PREM_RW, new_pt_paddr);
    }

    new_dir_vaddr->entry[PD_LAST_INDEX] = PDE(T_SELF_REF_PERM, new_dir_paddr);

    return new_dir_paddr;
}

void __del_page_table(pid_t pid, uintptr_t mount_point){

    x86_page_t *page_dir      = (x86_page_t *)((mount_point) | (0x3FF << 12));

    for(uint32_t i=0; i<PD_LAST_INDEX; ++i){

        x86_pte_t pde = page_dir->entry[i];
        if(!pde || !(pde & PG_PRESENT)){
            continue;
        }

        x86_page_t *page_table   = (x86_page_t *)(mount_point | (i << 12));
        for(uint32_t j=0; j<PG_MAX_ENTRIES; ++j){

            x86_pte_t pte  = page_table->entry[j];
            if(pte & PG_PRESENT){
                pmm_free_page(pid, PG_ENTRY_ADDR(pte));
            }

        }
        pmm_free_page(pid, PG_ENTRY_ADDR(pde));

    }
    pmm_free_page(pid, PG_ENTRY_ADDR(page_dir->entry[PD_LAST_INDEX]));
}

void *dup_page_table(pid_t pid){
    return __dup_page_table(pid, PD_REFERENCED);
}


void setup_task_mem(struct task_struct *task, uintptr_t mount){

    pid_t pid     = task->pid;
    void *new_dir = __dup_page_table(pid, mount);

    vmm_mount_pg_dir(PD_MOUNT_2, new_dir);

    for(uint32_t i=K_STACK_START; i <= K_STACK_TOP; i+=PG_SIZE){

        volatile x86_pte_t *pte = __MOUNTED_PTE(PD_MOUNT_2, i);

        cpu_invplg(pte);

        x86_pte_t p   = *pte;

        uintptr_t new_page = vmm_dup_page(pid, PG_ENTRY_ADDR(p));

        *pte = ((p & 0xFFF) | new_page);

    }

    task->page_table = new_dir;

}


pid_t dup_proc(){

    pid_t   pid                  = alloc_pid();
    struct task_struct *new_task = get_task(pid);
    memset(new_task, 0, sizeof(struct task_struct));
    new_task->pid          = pid;
    new_task->state        = TASK_CREATED;
    new_task->created      = clock_systime();
    new_task->mm.user_heap = __current->mm.user_heap;
    new_task->regs         = __current->regs;
    new_task->parent       = __current;

    list_init_head(&new_task->mm.regions.head);

    setup_task_mem(new_task, PD_REFERENCED);

    do{

        if(list_empty(&__current->mm.regions.head))break;

        struct mm_region *pos, *n;
        list_for_each(pos, n, &__current->mm.regions.head, head){
            region_add(new_task, pos->start, pos->end, pos->attr);

            if(pos->attr & REGION_WSHARED){
                continue;
            }

            uintptr_t start_p = PG_ALIGN(pos->start);
            uintptr_t end_p   = PG_ALIGN(pos->end);
            for(uint32_t i=start_p; i<end_p; i+=PG_SIZE){

                x86_pte_t *c_pte = __MOUNTED_PTE(PD_MOUNT_1, i);
                x86_pte_t *n_pte = __MOUNTED_PTE(PD_MOUNT_2, i);
                cpu_invplg(c_pte);
                cpu_invplg(n_pte);

                if(pos->attr == REGION_RSHARED){
                    *c_pte = *c_pte & ~PG_WRITE;
                    *n_pte = *c_pte & ~PG_WRITE;
                }else{
                    *n_pte = 0;
                }

            }

        }

    }while(0);

    vmm_unmount_pg_dir(PD_MOUNT_2);

    new_task->regs.eax   = 0;

    commit_task(new_task);

    // push_task(&new_task);

    return new_task->pid;

}

 __DEFINE_SYSTEMCALL_0(pid_t, fork){
    return dup_proc();
}
 __DEFINE_SYSTEMCALL_0(pid_t, getpid){
     return __current->pid;
 }
 __DEFINE_SYSTEMCALL_0(pid_t, getppid){
     return __current->parent->pid;
 }





