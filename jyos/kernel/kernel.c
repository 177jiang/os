#include <stdint.h>
#include <jyos/tty/tty.h>
#include <arch/x86/boot/multiboot.h>
#include <arch/x86/gdt.h>
#include <arch/x86/idt.h>
#include <jyos/constant.h>
#include <libc/stdlib.h>
#include <libc/stdio.h>
#include <stddef.h>
#include <libc/stdio.h>
#include <jyos/mm/kalloc.h>

extern uint8_t __kernel_start;


#define _init_ks_size (4 * 1024 * 4)
char _init_kernel_stack[_init_ks_size];

uint32_t _init_kernel_esp = (uint32_t)_init_kernel_stack + _init_ks_size;


// void __test_mb_info(multiboot_info_t *mb_info){
//
//   // uint32_t map_size = mb_info->mmap_length / sizeof(multiboot_memory_map_t);
//
//   size_t kernel_pg_count = (uintptr_t)(&__kernel_end - &__kernel_start) >> 12;
//   size_t hhk_pg_count = ((uintptr_t)(&__init_hhk_end) - MEM_1MB) >> 12;
//
//   printf("Mem: %d -- %d \n", mb_info->mem_lower, mb_info->mem_upper);
//   printf("Allocated %d page for kernel \n", kernel_pg_count);
//   printf("Allocated %d page for hhk \n", hhk_pg_count);
//
//   while(1);
//
// }

// void _kernel_init() {
//
//    _tty_init((void *)VGA_BUFFER_PADDR);
//    printf("Initializing kernel .......................\n");
//    // __test_mb_info(mb_info);
//    printf("---------------Initialization Done----------------\n");
//
//    // clear_screen();
// }

int _kernel_main() {

  // printf("this is a test %d, %s, %x\n", 1, "str", 16);
  // printf("this is a test %d, %s, %x\n", -1, "stsdasdr", 16);
  // printf("this is a test %d, %s, %x\n", 1123123, "str", 16);
  // printf("this is a test %d, %s, %x\n", -199922, "str", 16);
  //
  // while(1);

  // char str[128] = "";
  // __itoa_internal(-1, str, 10, 0);
  // tty_put_str(str);

  // __asm__("int $0");
  // __asm__("int $0");

  // beep();

  char *alloc_test = (char *) kmalloc(64);
  for(int i=0; i<20; ++i){
    alloc_test[i] = i;
  }
  for(int i=0; i<20; ++i){
    printf("%u ", alloc_test[i]);
  }

  kfree(alloc_test);

  while(1);

  while (1) play_cxk_gif();

}


