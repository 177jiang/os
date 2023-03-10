
#ifndef __jyos_mutex_h_
#define __jyos_mutex_h_

#include "semaphore.h"


typedef struct sem_t mutex_t;

static inline void mutex_init(mutex_t *mutex) {
    sem_init(mutex, 1);
}

static inline unsigned int mutex_on_hold(mutex_t *mutex) {
    return !atomic_load(&mutex->counter);
}

static inline void mutex_lock(mutex_t *mutex) {
    sem_wait(mutex);
}

static inline void mutex_unlock(mutex_t *mutex) {
    sem_post(mutex);
}

#endif
