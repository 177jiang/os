#include <jyos/mm/page.h>
#include <jyos/mm/vmm.h>
#include <jyos/mm/dmm.h>

#include <jyos/spike.h>

#include <stdint.h>

#define CAST(type, a)               ((type)(a))

#define DMM_HEADER_SIZE             4
#define DMM_BOUNDRY                 4

#define DMM_SELF_ALLOC              0x1
#define DMM_PREV_ALLOC              0x2
#define DMM_FLAGS_MSK               0x3

#define DMM_GET_HEADER(addr)        ( *(CAST(uint32_t*, addr)) )
#define DMM_GET_SIZE(header)        (header & ~DMM_FLAGS_MSK)
#define DMM_GET_PREV(header)        (header & DMM_PREV_ALLOC)
#define DMM_GET_SELF(header)        (header & DMM_SELF_ALLOC)

#define DMM_MAKE(size, flags)       ( (size) | ((flags) & DMM_FLAGS_MSK) )

#define DMM_SET_HT(addr, header)    ( DMM_GET_HEADER(addr) = (header) )

#define DMM_GET_PREV_HEADER(addr)   (  *(CAST(uint32_t *, addr)-1) )


#define DMM_GET_RET_ADDR(addr)      (CAST(uint8_t *, addr) + DMM_HEADER_SIZE)

#define DMM_GET_TAIL(addr, size)    ( CAST(uint32_t *, (addr + size - DMM_HEADER_SIZE) ) )


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

void *jbrk(heap_context_t *heap, size_t size){

    if(size == 0)return heap->end;

    pid_t pid = 0;

    char *new_end = (char*)heap->end + ROUNDUP(size + DMM_HEADER_SIZE, DMM_HEADER_SIZE);

    if((uint32_t)new_end >= K_STACK_START){
        return 0;
    }

    uint32_t heap_base = PG_ALIGN(heap->end);

    if(heap_base != PG_ALIGN(new_end)){

        int alloc =
            vmm_alloc_pages(
                    pid,
                    (uint8_t*)heap_base + PG_SIZE,
                    ROUNDUP(size, PG_SIZE),
                    PG_PREM_RW,
                    0
            );

        if(!alloc) return 0;

    }

    uint8_t *old_end = heap->end;
    heap->end += size;
    return old_end;
}

void *_grow_heap(heap_context_t *heap, size_t size){

    size = ROUNDUP(size, DMM_BOUNDRY);

    uint8_t *start = jbrk(heap, size);

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

int dmm_init(heap_context_t *heap){

    assert_msg((uint32_t)heap->start % DMM_BOUNDRY == 0, "heap must align ");

    heap->end = heap->start;

    pid_t pid = 0;

    vmm_alloc_page(pid, heap->start, 0, PG_PREM_RW, 0);

    DMM_SET_HT(
            heap->start,
            DMM_MAKE(4, (DMM_PREV_ALLOC | DMM_SELF_ALLOC) )
    );

    DMM_SET_HT(
            heap->start + DMM_HEADER_SIZE,
            DMM_MAKE(0, DMM_PREV_ALLOC | DMM_SELF_ALLOC)
            );

    heap->end +=  DMM_HEADER_SIZE;

    return _grow_heap(heap, HEAP_INIT_SIZE) != 0;
}


void *jmalloc(heap_context_t *heap, size_t size){

    uint8_t *start = heap->start;
    uint8_t *end   = heap->end;
    size = ROUNDUP(size, DMM_BOUNDRY) + DMM_HEADER_SIZE;

    while(start < end) {
        uint32_t header  = DMM_GET_HEADER(start);
        uint32_t bk_size = DMM_GET_SIZE(header);
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

    uint8_t *hd     = (uint8_t *)addr - DMM_HEADER_SIZE;
    uint32_t header = DMM_GET_HEADER(hd);
    size_t size     = DMM_GET_SIZE(header);
    uint8_t *nhd    = hd + size;

    printf("%x", addr );

    assert_msg(((uintptr_t)addr < (uintptr_t)(-size)) && (((uintptr_t)addr & 0x3) == 0),
               "free(): invalid pointer");

    assert_msg(size > DMM_HEADER_SIZE && (size & ~0x3),
               "free(): invalid size");

    DMM_SET_HT(hd, header & ~DMM_SELF_ALLOC);
    DMM_SET_HT(hd + size - DMM_HEADER_SIZE, header & ~DMM_SELF_ALLOC);

    uint32_t next_header = DMM_GET_HEADER(nhd);
    DMM_SET_HT(nhd, next_header & ~DMM_PREV_ALLOC);

    _coalesce(hd);

}
