#ifndef __jyos_process_h_
#define __jyos_process_h_


#include <stdint.h>
#include <arch/x86/interrupts.h>
#include <clock.h>
#include <mm/mm.h>

#define KERNEL_PID   0

#define TASK_CREATED 0
#define TASK_RUNNING 1
#define TASK_STOPPED 2
#define TASK_TERMNAT 3
#define TASK_DESTROY 4

#define pid_t uint32_t 

struct proc_mm{
    heap_context_t      kernel_heap;
    heap_context_t      user_heap;
    struct mm_region    *region;
};

struct task_struct{

    pid_t               pid;
    pid_t               parent;
    isr_param           regs;
    struct proc_mm      mm;
    void                *page_table;
    time_t              created;
    time_t              parent_created;
    uint8_t             state;
    int32_t             exit_code;
    int32_t             k_status;

};

extern struct task_struct *__current;

void dup_proc();
pid_t alloc_pid();
void push_task(struct task_struct *task);
void destroy_task(pid_t pid);
struct task_struct *get_task(pid_t pid);
void terminate_task(int exit_code);

#endif
