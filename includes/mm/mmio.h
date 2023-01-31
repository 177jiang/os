#ifndef __jyos_mmio_h_
#define __jyos_mmio_h_

#include <stdint.h>

void *ioremap(uintptr_t paddr, unsigned int size);

void *iounmap(uintptr_t vaddr, unsigned int size);


#endif
