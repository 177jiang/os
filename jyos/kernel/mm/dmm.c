#include <mm/page.h>
#include <mm/vmm.h>
#include <mm/dmm.h>

#include <spike.h>
#include <status.h>
#include <process.h>

#include <stdint.h>


void *_syscall_brk(size_t size){

    heap_context_t *heap = &__current->mm.user_heap;

    mutex_lock(&heap->lock);
    void *ad = jbrk(heap, size);
    mutex_unlock(&heap->lock);

    return ad;
}

void *_syscall_sbrk(void *addr){

    heap_context_t *heap = &__current->mm.user_heap;

    mutex_lock(&heap->lock);
    void *ad = jsbrk(heap, addr);
    mutex_unlock(&heap->lock);

    return ad;
}


void *jbrk(heap_context_t *heap, size_t size){

    if(size == 0)return heap->end;


    char *new_end = (char*)heap->end + ROUNDUP(size + DMM_HEADER_SIZE, DMM_HEADER_SIZE);

    if((uint32_t)new_end >= heap->max_addr || new_end < heap->end){
        __current->k_status = INVLDPTR;
    }

    uint32_t heap_base = PG_ALIGN(heap->end);

    if(heap_base != PG_ALIGN(new_end)){

        int alloc =
            vmm_alloc_pages(
                    __current->pid,
                    (uint8_t*)heap_base + PG_SIZE,
                    ROUNDUP(size, PG_SIZE),
                    PG_PREM_RW,
                    0
            );

        if(!alloc){
            __current->k_status = HEAPFULL;
            return 0;
        }

    }

    uint8_t *old_end = heap->end;
    heap->end += size;
    return old_end;
}

int jsbrk(heap_context_t *heap, uint8_t *addr){
    return jbrk(heap, (size_t)(addr - heap->end)) != 0;
}

int dmm_init(heap_context_t *heap){

    assert_msg((uint32_t)heap->start % DMM_BOUNDRY == 0, "heap must align ");

    heap->end = heap->start;

    mutex_init(&heap->lock);


    return vmm_alloc_page(__current->pid, heap->end, 0, PG_PREM_RW, 0);

}


