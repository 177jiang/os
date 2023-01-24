#ifndef __jyos_process_h_
#define __jyos_process_h_


#define KERNEL_PID   0

#define TASK_STOPPED 0
#define TASK_RUNNING 1
#define TASK_TERMNAT 2
#define TASK_DESTROY 4
#define TASK_BLOCKED 8
#define TASK_CREATED 16

#define TASK_TERMMASK 0x6

#define pid_t int32_t

#define SIZEOF_REGS        (76)

#define TASK_OFF_USTACK    (SIZEOF_REGS)
#define TASK_OFF_PAGETABLE (TASK_OFF_USTACK + 4)

#define SIG_OFF_HANDLER     (SIZEOF_REGS)
#define SIG_OFF_NUM         (SIG_OFF_HANDLER + 4)

#ifndef __ASM_S_
#include <arch/x86/interrupts.h>
#include <datastructs/jlist.h>
#include <mm/mm.h>
#include <mm/page.h>
#include <signal.h>
#include <clock.h>

struct proc_mm{
    heap_context_t      user_heap;
    struct mm_region    regions;
};

struct sig_struct{
    isr_param   prev_regs;
    void        *signal_handler;
    int         sig_num;
};

struct task_struct{

    isr_param               regs;

    uint32_t                *user_stack_top;

    void                    *page_table;

    pid_t                   pid;
    pid_t                   pgid;
    time_t                  created;
    uint8_t                 state;
    int32_t                 exit_code;
    int32_t                 k_status;

    signal_t                sig_pending;
    signal_t                sig_mask;
    void                    *signal_handlers[_SIG_MAX];

    struct proc_mm          mm;

    struct task_struct      *parent;
    struct list_header      children;
    struct list_header      siblings;
    struct list_header      group;

    struct timer*           timer;

};

extern volatile struct task_struct *__current;

pid_t dup_proc();

pid_t alloc_pid();

void init_task_user_space(struct task_struct *task);

struct task_struct *alloc_task();

void push_task(struct task_struct *task);

pid_t destroy_task(pid_t pid);

struct task_struct *get_task(pid_t pid);

void terminate_task(int exit_code);

Pysical(void *) dup_page_table(pid_t pid);

void task_init(struct task_struct *task);

void setup_task_page_table(struct task_struct *task, uintptr_t mount);

void setup_task_mem_region(struct mm_region *from, struct mm_region *to);

void new_task();

int orphaned_task(pid_t pid);

void commit_task(struct task_struct *task);

#endif

#endif
