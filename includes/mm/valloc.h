#ifndef __jyos_valloc_h_
#define __jyos_valloc_h_

void *valloc(unsigned int size);
void *valloc_dma(unsigned int size);

void vfree(void *vaddr);
void free_dma(void *vaddr);

void valloc_init();


#endif
