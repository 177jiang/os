#ifndef __jyos_sched_h_
#define __jyos_sched_h_

#define SCHED_TIME_SLICE 1500

struct scheduler {

    struct task_struct *tasks;
    int                 current_index;
    unsigned int        task_len;

};

void sched_init();

void schedule();

void schedule_yield();

#endif
