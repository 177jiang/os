
#include <datastructs/semaphore.h>
#include <stdatomic.h>
#include <stdint.h>

void sem_init(struct sem_t *sem, unsigned int initc){
    sem->counter = ATOMIC_VAR_INIT(initc);
}
void sem_wait(struct sem_t *sem){
    while(!atomic_load(&sem->counter)){

    }
    atomic_fetch_sub(&sem->counter, 1);
}

void sem_post(struct sem_t *sem){
    atomic_fetch_add(&sem->counter, 1);
}
