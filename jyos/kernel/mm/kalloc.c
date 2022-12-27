#include <mm/kalloc.h>
#include <libc/string.h>
#include <mm/dmm.h>

extern uint8_t __kernel_heap_start;

heap_context_t __kalloc_heap;

int kalloc_init(){
    __kalloc_heap.start =  sym_vaddr(__kernel_heap_start);
    __kalloc_heap.end   = 0;
    return dmm_init(&__kalloc_heap);
}

void *kmalloc(size_t size){
    return jmalloc(&__kalloc_heap, size);
}

void *kcalloc(size_t size){
    void *addr = kmalloc(size);
    if(!addr)return 0;

    memset(addr, 0, size);

    return addr;
}

void kfree(void *addr){
    jfree(addr);
}

void __debug_kalloc(){
    printf_("start :%x     end :%x\n", __kalloc_heap.start, __kalloc_heap.end);
}
