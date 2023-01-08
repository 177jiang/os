#include <process.h>
#include <mm/vmm.h>
#include <hal/cpu.h>
#include <hal/apic.h>
#include <status.h>
#include <sched.h>
#include <spike.h>
#include <timer.h>
#include <syscall.h>

#define MAX_TASKS        512

struct task_struct temp_task = {
    .pid = 0,
    .parent = -1,
};

struct task_struct *__current = &temp_task;

extern void __task_table;

struct scheduler sched_ctx;

void sched_init(){

    uint32_t pg_size = ROUNDUP(sizeof(struct task_struct) * MAX_TASKS, (PG_SIZE));
    uint32_t tvaddr  = sym_vaddr(__task_table);
    assert_msg(
            vmm_alloc_pages(0, tvaddr, pg_size, PG_PREM_RW, PP_FGPERSIST),
            "Failed to alloc tasks table"
            );

    sched_ctx = (struct scheduler){
        .tasks         = (struct task_struct *)(tvaddr),
        .current_index = 0,
        .task_len      = 0
    };

}

void switch_to(struct task_struct *new_task){

    if(!(__current->state & ~TASK_RUNNING)){
        __current->state = TASK_STOPPED;
    }

    new_task->state  = TASK_RUNNING;

    if(__current->page_table != new_task->page_table){
        __current  = new_task;
        cpu_lcr3(__current->page_table);

    }else{
        __current  = new_task;
    }


    apic_done_servicing();

    asm volatile (
            "pushl %0\n"
            "jmp soft_iret\n"
            ::"r"(&__current->regs)
            :"memory"
    );

}

uint32_t __schedule_algorithm() {
    return 1;
}

void schedule(){

    if (!sched_ctx.task_len) {
        return;
    }

    int p_task  = sched_ctx.current_index;
    int i       = p_task;
    struct task_struct *n_task;

    do {

        i = (i + 1);
        if(i > sched_ctx.task_len) i = 1;
        n_task = (sched_ctx.tasks + i);

    }while((n_task->state != TASK_STOPPED) && (i != p_task));

    sched_ctx.current_index = i;

    switch_to(n_task);

}

void commit_task(struct task_struct *task){

    list_init_head(&task->children);

    if(task->parent){
        list_append(&task->parent->children, &task->siblings);
    }else{
        task->parent = &sched_ctx.tasks[1];
    }

    task->state = TASK_STOPPED;

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

void push_task(struct task_struct *task){

    int index = task->pid;
    if(index <= 0 || index > sched_ctx.task_len + 1){
        __current->k_status = INVLDPID;
        return;
    }

    task->parent           = __current;
    task->state            = TASK_STOPPED;
    sched_ctx.tasks[index] = *task;

    if(index == sched_ctx.task_len + 1){
        ++sched_ctx.task_len;
    }

}

pid_t destroy_task(pid_t pid){

    if(pid <= 1 || pid > sched_ctx.task_len){
        __current->k_status = INVLDPID;
        return;
    }

    struct task_struct *task = sched_ctx.tasks + pid;
    task->state              = TASK_DESTROY;

    list_delete(&task->siblings);

    if(!list_empty(&task->mm.regions.head)){
        struct mm_region *pos, *n;
        list_for_each(pos, n, &task->mm.regions.head, head){

            kfree(pos);

        }
    }

    vmm_mount_pg_dir(PD_MOUNT_2, task->page_table);

    __del_page_table(pid, PD_MOUNT_2);

    vmm_unmount_pg_dir(PD_MOUNT_2);

    return pid;
}

void terminate_task(int exit_code){

    __current->state     = TASK_TERMNAT;

    __current->exit_code = exit_code;

    schedule();

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

    return (parent->state & TASK_TERMNAT) || parent->created > task->created;

}

static void task_timer_callback(struct task_struct *task){
    task->timer = 0;
    task->state = TASK_STOPPED;
}
__DEFINE_SYSTEMCALL_1(unsigned int, sleep, unsigned int, seconds){

    if(!seconds)return 0;

    if(__current->timer){
        return __current->timer->counter / timer_context()->running_frequency;
    }

    struct timer *timer = timer_run_second(seconds, task_timer_callback, __current, 0);
    __current->timer    = timer;
    __current->regs.eax = seconds;
    __current->state    = TASK_BLOCKED;

    schedule();

}

__DEFINE_SYSTEMCALL_0(void, yield){
    schedule();
}

__DEFINE_SYSTEMCALL_1(void, _exit, int, exit_code){
    terminate_task(exit_code);
}

__DEFINE_SYSTEMCALL_1(pid_t, wait, int, *state){

    if(list_empty(&__current->children)){
        return -1;
    }

    struct task_struct *pos, *n;

    while(1){

        list_for_each(pos, n, &__current->children, siblings){

            if(pos->state == TASK_TERMNAT){

                *state = pos->exit_code;

                return destroy_task(pos->pid);
            }

        }
    }

    /* never reach here */
    return  0;
}













