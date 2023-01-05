#ifndef __jyos_dmm_h_
#define __jyos_dmm_h_

#include <mm/page.h>
#include <mm/vmm.h>
#include <mm/mm.h>
#include <stdint.h>
#include <libc/stdio.h>
#include <stddef.h>


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
#define HEAP_INIT_SIZE    PG_SIZE


int dmm_init(heap_context_t *heap);

void *jbrk(heap_context_t *heap, size_t size);


#endif
