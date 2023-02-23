#include <spike.h>
#include <mm/kalloc.h>
#include <libc/string.h>
#include <mm/dmm.h>
#include <constant.h>
#include <spike.h>

extern uint8_t __kernel_heap_start;

void *_grow_heap(heap_context_t *heap, size_t size);
void *jmalloc(heap_context_t *heap, size_t size);
void _place_chunk(uint8_t *ptr, size_t size);
void *_coalesce(uint8_t *chunk_ptr);
void jfree(void *ptr);

static heap_context_t kernel_heap;

int kalloc_init(){

    kernel_heap.start          = K_HEAP_START;
    kernel_heap.end            = NULL;
    kernel_heap.max_addr       = (void*)TASK_TABLE_START;

    for(size_t i=0; i<K_HEAP_SIZE_MB >> 2; ++i){
        int a = vmm_set_mapping(
                PD_REFERENCED,
                (uint32_t)kernel_heap.start + (i << 22),
                0,
                PG_PREM_RW ,
                VMAP_NOMAP
        );
    }

    if(!dmm_init(&kernel_heap)) {
        return 0;
    }



    DMM_SET_HT(
            kernel_heap.start,
            DMM_MAKE(4, (DMM_PREV_ALLOC | DMM_SELF_ALLOC) )
    );


    DMM_SET_HT(
            kernel_heap.start + DMM_HEADER_SIZE,
            DMM_MAKE(0, DMM_PREV_ALLOC | DMM_SELF_ALLOC)
    );

    kernel_heap.end +=  DMM_HEADER_SIZE;

    return  _grow_heap(&kernel_heap, HEAP_INIT_SIZE);
}

void  _place_chunk(uint8_t *addr, size_t size){

    uint32_t header       = DMM_GET_HEADER(addr);
    uint32_t bk_size      = DMM_GET_SIZE(header);
    uint32_t prev         = DMM_GET_PREV(header);
    uint32_t first_header = DMM_MAKE(size, (prev|DMM_SELF_ALLOC));
    uint8_t *addr_n       = addr + size;
    DMM_SET_HT(addr, first_header);

    if(bk_size == size){
        uint32_t header_n = DMM_GET_HEADER(addr_n);
        DMM_SET_HT(addr_n, header_n | DMM_PREV_ALLOC);
    }else{
        uint32_t second_header =
            DMM_MAKE(
                 (bk_size - size),
                 DMM_PREV_ALLOC
            );
        DMM_SET_HT(addr_n, second_header);
        DMM_SET_HT(addr + bk_size - DMM_HEADER_SIZE, second_header);

        _coalesce(addr_n);
    }

}
void *_coalesce(uint8_t * addr){

    uint32_t header   = DMM_GET_HEADER(addr);
    uint32_t prev     = DMM_GET_PREV(header);
    uint32_t size     = DMM_GET_SIZE(header);

    uint32_t header_n = DMM_GET_HEADER(addr + size);
    uint32_t size_n   = DMM_GET_SIZE(header_n);
    uint32_t self_n   = DMM_GET_SELF(header_n);

    if(!prev && !self_n){
        uint32_t header_p   = DMM_GET_PREV_HEADER(addr);
        uint32_t size_p     = DMM_GET_SIZE(header_p);
        uint32_t new_size   = size + size_n + size_p;
        uint32_t new_prev   = DMM_GET_PREV(header_p);
        uint32_t new_header = DMM_MAKE(new_size, new_prev);
        uint8_t* new_addr   = addr - size_p;

        DMM_SET_HT(new_addr, new_header);
        DMM_SET_HT(new_addr + new_size - DMM_HEADER_SIZE, new_header);

        addr -= size_p;
    }else if(!prev){
        uint32_t header_p   = DMM_GET_PREV_HEADER(addr);
        uint32_t size_p     = DMM_GET_SIZE(header_p);
        uint32_t new_size   = size + size_p;
        uint32_t new_prev   = DMM_GET_PREV(header_p);
        uint32_t new_header = DMM_MAKE(new_size, new_prev);
        uint8_t* new_addr   = addr - size_p;

        DMM_SET_HT(new_addr, new_header);
        DMM_SET_HT(new_addr + new_size - DMM_HEADER_SIZE, new_header);

        addr -= size_p;
    }else if(!self_n){
        uint32_t new_size   = size + size_n;
        uint32_t new_header = DMM_MAKE(size, prev);

        DMM_SET_HT(addr, new_header);
        DMM_SET_HT(addr + new_size, new_header);

    }

    return addr;
}
void *_grow_heap(heap_context_t *heap, size_t size){
    size = ROUNDUP(size, DMM_BOUNDRY);

    uint8_t *start = jsbrk(heap, size, 0);

    if(!start)return 0;


    uint32_t old_header = DMM_GET_HEADER(start);
    uint32_t new_header = DMM_MAKE(size, DMM_GET_PREV(old_header));

    DMM_SET_HT(start, new_header);

    DMM_SET_HT(
            DMM_GET_TAIL(start, size),
            new_header
            );

    DMM_SET_HT(
            start + size ,
            DMM_MAKE(0, DMM_SELF_ALLOC )
            );

    return _coalesce(start);
}

void *jmalloc(heap_context_t *heap, size_t size){

    if(!size)return 0;
    uint8_t *start = heap->start;
    uint8_t *end   = heap->end;
    size = ROUNDUP(size, DMM_BOUNDRY) + DMM_HEADER_SIZE;

    while(start < end) {
        uint32_t header  = DMM_GET_HEADER(start);
        uint32_t bk_size = DMM_GET_SIZE(header);

        if(!bk_size && DMM_GET_SELF(header)){
            break;
        }

        if(!DMM_GET_SELF(header) && bk_size >= size){
            _place_chunk(start, size);
            return (start + DMM_HEADER_SIZE);
        }
        start += bk_size;
    }
    start = _grow_heap(heap, size);
    if(!start)return 0;

    _place_chunk(start, size);
    return (start + DMM_HEADER_SIZE);
}

void jfree(void *addr){

    if(!addr)return ;

    mutex_lock(&kernel_heap.lock);

    uint8_t *hd     = (uint8_t *)addr - DMM_HEADER_SIZE;
    uint32_t header = DMM_GET_HEADER(hd);
    size_t size     = DMM_GET_SIZE(header);
    uint8_t *nhd    = hd + size;

    assert_msg(((uintptr_t)addr < (uintptr_t)(-size)) &&
               !((uintptr_t)addr & 0x3),
               "free(): invalid pointer");

    assert_msg(size > DMM_HEADER_SIZE && (size & ~0x3),
               "free(): invalid size");


    DMM_SET_HT(hd, header & ~DMM_SELF_ALLOC);
    DMM_SET_HT(hd + size - DMM_HEADER_SIZE, header & ~DMM_SELF_ALLOC);

    uint32_t next_header = DMM_GET_HEADER(nhd);
    DMM_SET_HT(nhd, next_header & ~DMM_PREV_ALLOC);

    _coalesce(hd);

    mutex_unlock(&kernel_heap.lock);

}

void *kmalloc(size_t size){
    mutex_lock(&kernel_heap.lock);
    void*res =  jmalloc(&kernel_heap, size);
    mutex_unlock(&kernel_heap.lock);
    return res;
}

void *kcalloc(size_t size){

    if(size == 0)return 0;

    void *addr = kmalloc(size);


    if(!addr)return 0;

    memset(addr, 0, size);

    return addr;
}

void kfree(void *addr){
    jfree(addr);
}

