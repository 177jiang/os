#include <mm/page.h>
#include <mm/vmm.h>
#include <mm/dmm.h>

#include <spike.h>
#include <status.h>
#include <process.h>
#include <syscall.h>

#include <stdint.h>


__DEFINE_SYSTEMCALL_1(void*, sbrk, size_t, size) {

    heap_context_t *heap = &__current->mm.user_heap;

    mutex_lock(&heap->lock);
    void *ad = jsbrk(heap, size, PG_ALLOW_USER);
    mutex_unlock(&heap->lock);

    return ad;
}

__DEFINE_SYSTEMCALL_1(int, brk, void*, addr) {

    heap_context_t *heap = &__current->mm.user_heap;

    mutex_lock(&heap->lock);
    void *ad = jbrk(heap, addr, PG_ALLOW_USER);
    mutex_unlock(&heap->lock);

    return ad;
}


int jbrk(heap_context_t *heap, void *addr, int user){

    return -(jsbrk(heap, ((uint8_t*)addr - heap->end), user) == (void*)-1);

}

void *jsbrk(heap_context_t *heap, size_t size, int user){

    if(size == 0)return heap->end;

    char *new_end = (char*)heap->end + ROUNDUP(size + DMM_HEADER_SIZE, DMM_HEADER_SIZE);

    if((uint32_t)new_end >= heap->max_addr || new_end < heap->end){
        __current->k_status = INVLDPTR;
        return (void*)-1;
    }

    uint32_t heap_base = PG_ALIGN(heap->end);

    uint32_t diff = PG_ALIGN(new_end) - heap_base;

    for(uint32_t i=0; i<diff; i+=PG_SIZE){
        vmm_set_mapping(
                PD_REFERENCED,
                heap_base + PG_SIZE + i,
                0,
                PG_WRITE | user,
                VMAP_NULL
        );
    }



    uint8_t *old_end = heap->end;
    heap->end += size;
    return old_end;
}


int dmm_init(heap_context_t *heap){

    assert_msg((uint32_t)heap->start % DMM_BOUNDRY == 0, "heap must align ");

    heap->end = heap->start;

    mutex_init(&heap->lock);

    int p = PG_ALLOW_USER;
    if(heap->end >= K_HEAP_START){
        p = 0;
    }
    return vmm_set_mapping(PD_REFERENCED, heap->end, 0,  PG_WRITE | p, VMAP_NULL) != 0;

}



