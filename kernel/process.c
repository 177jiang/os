#include <mm/vmm.h>
#include <mm/pmm.h>
#include <hal/cpu.h>
#include <libc/stdio.h>
#include <libc/string.h>

#include <process.h>
#include <clock.h>
#include <syscall.h>
#include <status.h>


void init_task_user_space(struct task_struct *task){

    vmm_mount_pg_dir(PD_MOUNT_1, task->page_table);

    /* user stack */
    region_add(&task->mm.regions,
               U_STACK_END,
               U_STACK_TOP,
               (REGION_RW | REGION_RSHARED));

    /* user stack in page_table just set 0 , if used will alloc in page_fault*/
    for(uint32_t i=PG_ALIGN(U_STACK_END); i<=PG_ALIGN(U_STACK_TOP); i+=PG_SIZE){
        vmm_set_mapping(PD_MOUNT_1, i, 0, (PG_ALLOW_USER | PG_WRITE), VMAP_NULL);
    }

    vmm_unmount_pg_dir(PD_MOUNT_1);

}

Pysical(void *) __dup_page_table(pid_t pid, uint32_t mount_point){

    /* moint_point is a pg_addr in dir*/

    x86_page_t *new_dir_vaddr = PG_MOUNT_1;
    void       *new_dir_paddr = pmm_alloc_page(pid, PP_FGPERSIST);
    vmm_set_mapping( PD_REFERENCED, new_dir_vaddr, new_dir_paddr, PG_PREM_RW, VMAP_NULL);

    x86_page_t *page_dir      = (x86_page_t *)((mount_point) | (0x3FF << 12));

    size_t kspace = PD_INDEX(KERNEL_VSTART);
    for(uint32_t i=0; i<PD_LAST_INDEX; ++i){

        x86_pte_t pde = page_dir->entry[i];
        if(!pde || i>=kspace || !(pde & PG_PRESENT)){
            new_dir_vaddr->entry[i] = pde;
            continue;
        }

        x86_page_t *new_pt_vaddr = PG_MOUNT_2;
        void       *new_pt_paddr = pmm_alloc_page(pid, PP_FGPERSIST);
        vmm_set_mapping( PD_REFERENCED, new_pt_vaddr, new_pt_paddr, PG_PREM_RW, VMAP_NULL);

        x86_page_t *page_table   = (x86_page_t *)(mount_point | (i << 12));

        for(uint32_t j=0; j<PG_MAX_ENTRIES; ++j){

            x86_pte_t pte          = page_table->entry[j];

            pmm_ref_page(pid, PG_ENTRY_ADDR(pte));

            new_pt_vaddr->entry[j] = pte;
        }

        new_dir_vaddr->entry[i] = PDE(PG_ENTRY_FLAGS(pde), new_pt_paddr);
    }

    new_dir_vaddr->entry[PD_LAST_INDEX] = PDE(T_SELF_REF_PERM, new_dir_paddr);

    return new_dir_paddr;
}

void __del_page_table(pid_t pid, uintptr_t mount_point){

    x86_page_t *page_dir  = (x86_page_t *)((mount_point) | (0x3FF << 12));

    for(uint32_t i=0; i<PD_INDEX(KERNEL_VSTART); ++i){

        x86_pte_t pde = page_dir->entry[i];
        if(!pde || !(pde & PG_PRESENT)){
            continue;
        }

        x86_page_t *page_table   = (x86_page_t *)(mount_point | (i << 12));
        for(uint32_t j=0; j<PG_MAX_ENTRIES; ++j){

            x86_pte_t pte  = page_table->entry[j];
            if( (pte & PG_PRESENT) ){
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


void setup_task_page_table(struct task_struct *task, uintptr_t mount){

    pid_t pid     = task->pid;
    void *new_dir = __dup_page_table(pid, mount);

    if(pid == 1){
        task->page_table = new_dir;
        return ;
    }

    vmm_mount_pg_dir(PD_MOUNT_1, new_dir);

    for(uint32_t i=K_STACK_START; i <= K_STACK_TOP; i+=PG_SIZE){

        volatile x86_pte_t *pte = __MOUNTED_PTE(PD_MOUNT_1, i);

        cpu_invplg(pte);

        x86_pte_t p = *pte;

        uintptr_t new_page = vmm_dup_page(pid, PG_ENTRY_ADDR(p));

        pmm_free_page(pid, PG_ENTRY_ADDR(p));

        *pte = PTE((p&0xFFF), new_page);

    }

    task->page_table = new_dir;

}

void setup_task_mem_region(struct mm_region *from,
                           struct mm_region *to,
                           uintptr_t mount){

    region_copy(from, to);

    struct mm_region *pos, *n;
    list_for_each(pos, n, &to->head, head){

        if( (pos->attr & REGION_WSHARED) ){
            continue;
        }

        uintptr_t start_p = PG_ALIGN(pos->start);
        uintptr_t end_p   = PG_ALIGN(pos->end);

        for(uint32_t i=start_p; i<=end_p; i+=PG_SIZE){

            x86_pte_t *c_pte = __MOUNTED_PTE(PD_REFERENCED, i);
            x86_pte_t *n_pte = __MOUNTED_PTE(mount, i);
            cpu_invplg(n_pte);

            if( (pos->attr & REGION_MODE_MASK) == REGION_RSHARED){

                cpu_invplg(c_pte);

                cpu_invplg(i);

                *c_pte = *c_pte & ~PG_WRITE;
                *n_pte = *n_pte & ~PG_WRITE;
            }else{
                *n_pte = 0;
            }

        }
    }

}


pid_t dup_proc(){

    struct task_struct *new_task = alloc_task();

    new_task->mm.user_heap       = __current->mm.user_heap;
    new_task->regs               = __current->regs;
    new_task->parent             = __current;
    memcpy(new_task->fdtable, 
           __current->fdtable,
           sizeof(struct v_fdtable)
    );

    setup_task_page_table(new_task, PD_REFERENCED);
    setup_task_mem_region(&__current->mm.regions,
                          &new_task->mm.regions,
                          PD_MOUNT_1);

    vmm_unmount_pg_dir(PD_MOUNT_1);

    new_task->regs.eax   = 0;

    commit_task(new_task);

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
__DEFINE_SYSTEMCALL_0(pid_t, getpgid){
    return __current->pgid;
}

__DEFINE_SYSTEMCALL_2(int, setpgid, pid_t, pid, pid_t, pgid){

    struct task_struct *task = pid > 0 ? get_task(pid) : __current;

    if(!task || task->pid == task->pgid){
        __current->k_status = EINVAL;
        return -1;
    }

    struct task_struct *pg_task = pgid > 0 ? get_task(pgid) : __current;

    if(!pg_task){
        __current->k_status = EINVAL;
        return -1;
    }

    list_delete(&task->group);
    list_append(&pg_task->group, &pg_task->group);

    task->pgid = pg_task->pid;

    return 0;

}




