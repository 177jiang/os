#ifndef __jyos_dmm_h_
#define __jyos_dmm_h_

#include <jyos/mm/page.h>
#include <jyos/mm/vmm.h>
#include <stdint.h>
#include <libc/stdio.h>
#include <stddef.h>

#define HEAP_INIT_SIZE    PG_SIZE

typedef struct {

    uint8_t *start;
    uint8_t *end;

}heap_context_t;

int dmm_init(heap_context_t *heap);

void *jbrk(heap_context_t *heap, size_t size);

void *jmalloc(heap_context_t *heap, size_t size);

void jfree(void *ptr);

#endif
