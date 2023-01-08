#ifndef __jyos_process_h_
#define __jyos_process_h_


#include <stdint.h>
#include <arch/x86/interrupts.h>
#include <clock.h>
#include <mm/mm.h>
#include <mm/page.h>
#include <datastructs/jlist.h>

#define KERNEL_PID   0

#define TASK_STOPPED 0
#define TASK_RUNNING 1
#define TASK_TERMNAT 2
#define TASK_DESTROY 4
#define TASK_BLOCKED 8
#define TASK_CREATED 16

#define TASK_TERMMASK 0x6

#define pid_t uint32_t 

struct proc_mm{
    heap_context_t      user_heap;
    struct mm_region    regions;
};

struct task_struct{

    pid_t                   pid;

    isr_param               regs;

    void                    *page_table;

    time_t                  created;

    uint8_t                 state;

    int32_t                 exit_code;

    int32_t                 k_status;

    struct task_struct      *parent;

    struct list_header      children;

    struct list_header      siblings;

    struct timer*           timer;

    struct proc_mm          mm;


};

extern struct task_struct *__current;

pid_t dup_proc();

pid_t alloc_pid();

void push_task(struct task_struct *task);

pid_t destroy_task(pid_t pid);

struct task_struct *get_task(pid_t pid);

void terminate_task(int exit_code);

Pysical(void *) dup_page_table(pid_t pid);

void task_init(struct task_struct *task);

void setup_task_mem(struct task_struct *task, uintptr_t mount);

void new_task();

int orphaned_task(pid_t pid);

void commit_task(struct task_struct *task);

#endif
