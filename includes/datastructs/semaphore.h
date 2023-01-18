#ifndef __jyos_semaphore_h_
#define __jyos_semaphore_h_

#include <stdatomic.h>

struct sem_t {
    _Atomic unsigned int counter;
    // FUTURE: might need a waiting list
};

void sem_init(struct sem_t *sem, unsigned int initial);

void sem_wait(struct sem_t *sem);

void sem_post(struct sem_t *sem);

#endif
