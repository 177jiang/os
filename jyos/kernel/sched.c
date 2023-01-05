#include <process.h>
#include <mm/vmm.h>
#include <hal/cpu.h>
#include <hal/apic.h>
#include <status.h>
#include <sched.h>
#include <spike.h>

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

    __current = &temp_task;

}

void schedule(){
    if(sched_ctx.task_len == 0){
        return;
    }
    int p_task  = sched_ctx.current_index;
    int i       = p_task;
    struct task_struct *n_task;

    do {

        i = (i + 1);
        if(i > sched_ctx.task_len) i = 1;
        n_task = (sched_ctx.tasks + i);

    }while((n_task->state != TASK_STOPPED
                && n_task->state != TASK_CREATED) &&
            i != p_task);

    __current->state        = TASK_STOPPED;
    sched_ctx.current_index = i;

    n_task->state            = TASK_RUNNING;
    __current                = n_task;

    cpu_lcr3(__current->page_table);

    static FIRST_INIT_TASK_DOT_NEED_DO_THIS = 0;
    FIRST_INIT_TASK_DOT_NEED_DO_THIS++;
    if(FIRST_INIT_TASK_DOT_NEED_DO_THIS > 1){
        apic_done_servicing();
    }


    asm volatile (
            "pushl %0\n jmp soft_iret\n"::
            "r"(&__current->regs): "memory");

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

    return i;
}

void push_task(struct task_struct *task){

    int index = task->pid;
    if(index <= 0 || index > sched_ctx.task_len + 1){
        __current->k_status = INVLDPID;
        return;
    }

    if(index == sched_ctx.task_len + 1){
        ++sched_ctx.task_len;
    }

    task->parent           = __current->pid;
    task->state            = TASK_CREATED;
    sched_ctx.tasks[index] = *task;

}

void destroy_task(pid_t pid){
    if(pid <= 0 || pid > sched_ctx.task_len){
        __current->k_status = INVLDPID;
        return;
    }
    sched_ctx.tasks[pid].state = TASK_DESTROY;
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
















