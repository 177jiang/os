#include <mm/vmm.h>
#include <mm/valloc.h>

#include <hal/cpu.h>
#include <hal/apic.h>

#include <status.h>
#include <sched.h>
#include <process.h>
#include <spike.h>
#include <types.h>
#include <timer.h>
#include <syscall.h>

#define MAX_TASKS        512


void check_sleepers();

volatile struct task_struct *__current;

extern void __task_table;

struct scheduler sched_ctx;

void sched_init(){

    uint32_t pg_size = ROUNDUP(sizeof(struct task_struct) * MAX_TASKS, (PG_SIZE));

    for(uint32_t i=0; i<=pg_size; i += PG_SIZE){
        uint32_t pa = pmm_alloc_page(KERNEL_PID, PP_FGPERSIST);
        vmm_set_mapping(PD_REFERENCED, TASK_TABLE_START+i, pa, PG_PREM_RW, VMAP_NULL);
    }

    sched_ctx = (struct scheduler){
        .tasks         = (struct task_struct *)(TASK_TABLE_START),
        .current_index = 0,
        .task_len      = 0
    };

}

void switch_to(struct task_struct *new_task){

    new_task->state  = TASK_RUNNING;

    tss_update_esp(new_task->regs.esp);

    apic_done_servicing();

    asm volatile (
            "pushl %0\n"
            "jmp __do_switch_to\n"
            ::"r"(new_task)
    );

}

uint32_t __schedule_algorithm() {
    return 1;
}

int can_schedule(struct task_struct *task){

    if(__SIGTEST(task->sig_pending, _SIGCONT)){
        __SIGCLEAR(task->sig_pending, _SIGSTOP);
    }else if(__SIGTEST(task->sig_pending, _SIGSTOP)){
        return 0;
    }
    return 1;
}

void check_sleepers(){

    struct task_struct *init_task = sched_ctx.tasks + 1;
    struct task_struct *pos, *n;
    time_t now = clock_systime();

    list_for_each(pos, n, &init_task->timer.sleepers, timer.sleepers){

        if(TASK_TERMINATED(pos->state)){
            goto __delete_slp;
        }

        time_t wtim = pos->timer.wakeup;
        time_t atim = pos->timer.alarm;

        if(wtim && now >= wtim){
            pos->timer.wakeup = 0;
            pos->state = TASK_STOPPED;
        }

        if(atim && now >= atim){
            pos->timer.alarm = 0;
            __SIGTEST(pos->sig_pending, _SIGALRM);
        }

        if(!wtim && !atim){
    __delete_slp:
            list_delete(&pos->timer.sleepers);
        }
    }

}

void schedule(){

    if (!sched_ctx.task_len) {
        return;
    }

    cpu_disable_interrupt();

    if(!(__current->state & ~TASK_RUNNING)){
        __current->state = TASK_STOPPED;
    }

    int p_task  = sched_ctx.current_index;
    int i       = p_task;
    struct task_struct *n_task;

    check_sleepers();

    do{

        do {

            i = (i + 1);
            if(i > sched_ctx.task_len) i = 1;
            n_task = (sched_ctx.tasks + i);

        }while((n_task->state != TASK_STOPPED) && (i != p_task));

    }while(!can_schedule(n_task));

    sched_ctx.current_index = i;

    switch_to(n_task);

}

void commit_task(struct task_struct *task){

    assert_msg(task==(sched_ctx.tasks + task->pid), "Task set error\n");

    if(task->state != TASK_CREATED){
        __current->k_status = EINVL;
        return ;
    }

    if(!task->parent){
        task->parent = (sched_ctx.tasks + 1);
    }

    list_append(&task->parent->children, &task->siblings);

    task->state = TASK_STOPPED;

}

struct task_struct *alloc_task(){

    pid_t pid = alloc_pid();
    assert_msg(pid!=-1, "Task alloc failed \n");

    struct task_struct *new_task = get_task(pid);

    memset(new_task, 0, sizeof(*new_task));

    new_task->pid               = pid;
    new_task->pgid              = pid;
    new_task->state             = TASK_CREATED;
    new_task->timer.created     = clock_systime();
    new_task->user_stack_top    = U_STACK_TOP;
    new_task->fdtable           = vzalloc(sizeof(struct v_fdtable));


    list_init_head(&new_task->mm.regions.head);
    list_init_head(&new_task->children);
    list_init_head(&new_task->group);
    list_init_head(&new_task->timer.sleepers);

    return new_task;
}

pid_t alloc_pid(){

    pid_t i = 1;

    for(; i <= sched_ctx.task_len &&
          sched_ctx.tasks[i].state != TASK_DESTROY;
          ++i) ;

    if(i == MAX_TASKS){
        __current->k_status = TASKFULL;
        return -1;
    }

    if(i >= sched_ctx.task_len){
        sched_ctx.task_len = i;
    }

    return i;
}


pid_t destroy_task(pid_t pid){

    if(pid <= 1 || pid > sched_ctx.task_len){
        __current->k_status = EINVL;
        return;
    }


    struct task_struct *task = sched_ctx.tasks + pid;
    task->state              = TASK_DESTROY;

    list_delete(&task->siblings);


    struct mm_region *pos, *n;
    list_for_each(pos, n, &task->mm.regions.head, head){
        kfree(pos);
    }

    vmm_mount_pg_dir(PD_MOUNT_1, task->page_table);

    __del_page_table(pid, PD_MOUNT_1);

    vmm_unmount_pg_dir(PD_MOUNT_1);

    return pid;
}

void terminate_task(int exit_code){

    __current->state     = TASK_TERMNAT;

    __current->exit_code = exit_code;

    __SIGSET(__current->parent->sig_pending, _SIGCHLD);

}


struct task_struct *get_task(pid_t pid){
    if(pid <= 0 || pid > sched_ctx.task_len){
        return 0;
    }
    return (sched_ctx.tasks + pid);
}

int orphaned_task(pid_t pid){

    if(!pid)return 0;
    if(pid > sched_ctx.task_len)return 0;

    struct task_struct *task   = sched_ctx.tasks + pid;
    struct task_struct *parent = task->parent;

    return (parent->state & TASK_TERMNAT) || parent->timer.created > task->timer.created;

}


__DEFINE_SYSTEMCALL_1(unsigned int, sleep, unsigned int, seconds){

    if(!seconds)return 0;


    if(__current->pid == 1)return 0;


    if(__current->timer.wakeup){
        return (__current->timer.wakeup - clock_systime()) / 1000U;
    }

    __current->timer.wakeup = clock_systime() + seconds * 1000U;

    if(list_empty(&__current->timer.sleepers)){

        list_append(&sched_ctx.tasks[1].timer.sleepers, &__current->timer.sleepers);

    }

    __current->regs.eax = seconds;
    __current->state    = TASK_BLOCKED;

    schedule();

}

pid_t _wait(pid_t wpid, int *status, int options){

    pid_t cur = __current->pid;
    int status_flags = 0;
    struct task_struct *proc, *n;

    if (list_empty(&__current->children)) {
        return -1;
    }

    wpid = wpid ? wpid : -__current->pgid;
    cpu_enable_interrupt();
repeat:
    list_for_each(proc, n, &__current->children, siblings) {

        if (!~wpid || proc->pid == wpid || proc->pgid == -wpid) {

            if (proc->state == TASK_TERMNAT && !options) {
                status_flags |= TEXITTERM;
                goto done;
            }
            if (proc->state == TASK_STOPPED && (options & WUNTRACED)) {
                status_flags |= TEXITSTOP;
                goto done;
            }
        }
    }
    if ((options & WNOHANG)) {
        return 0;
    }
    // 放弃当前的运行机会
    sched_yield();
    goto repeat;

done:

    cpu_disable_interrupt();
    status_flags |= TEXITSIG * !!proc->sig_doing;
    *status = proc->exit_code | status_flags;
    return destroy_task(proc->pid);
}

__DEFINE_SYSTEMCALL_0(void, yield){
    schedule();
}

__DEFINE_SYSTEMCALL_1(void, _exit, int, exit_code){

    terminate_task(exit_code);

    struct task_struct *task = __current;
    for(size_t i=0; i<VFS_FD_MAX; ++i){
        struct v_fd *fd = task->fdtable->fds[i];
        if(fd){
            vfs_close(fd->file);
        }
    }

    vfree(task->fdtable);

    schedule();

}

__DEFINE_SYSTEMCALL_3(pid_t, waitpid, pid_t, wp, int, *state, int, options){
    return _wait(wp, state, options);
}

__DEFINE_SYSTEMCALL_1(pid_t, wait, int, *state){
    return _wait(-1, state, 0);
}

__DEFINE_SYSTEMCALL_1(unsigned int, alarm, unsigned int, seconds){

    time_t patim = __current->timer.alarm;
    time_t now   = clock_systime();

    __current->timer.alarm = seconds ? (now + seconds * 1000) : 0;

    if(list_empty(&__current->timer.sleepers)){
        list_append(&(sched_ctx.tasks + 1)->timer.sleepers,
                    &__current->timer.sleepers);
    }

    return patim ? (patim - now )/1000 : 0;

}

__DEFINE_SYSTEMCALL_0(int, geterror){

    return __current->k_status;
}











