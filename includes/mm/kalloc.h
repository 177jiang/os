#ifndef __jyos_kmalloc_h_
#define __jyos_kmalloc_h_

#include <stddef.h>

int kalloc_init();

void *kmalloc(size_t size);

void *kcalloc(size_t size);

void kfree(void *addr);

void __debug_kalloc();

#endif
