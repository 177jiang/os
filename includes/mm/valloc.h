#ifndef __jyos_valloc_h_
#define __jyos_valloc_h_

void *valloc(unsigned int size);
void *vzalloc(unsigned int size);
void *vcalloc(unsigned int size, unsigned int count);

void *valloc_dma(unsigned int size);
void *vzalloc_dma(unsigned int size);
void *vcalloc_dma(unsigned int size, unsigned int count);

void vfree(void *vaddr);
void free_dma(void *vaddr);

void valloc_init();


#endif
