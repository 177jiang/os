#include <arch/x86/boot/multiboot.h>
#include <arch/x86/gdt.h>
#include <arch/x86/idt.h>

#include <mm/kalloc.h>
#include <tty/tty.h>
#include <constant.h>
#include <timer.h>
#include <clock.h>

#include <libc/stdlib.h>
#include <libc/stdio.h>
#include <libc/stdio.h>

#include <stdint.h>
#include <stddef.h>

#include <peripheral/keyboard.h>
#include <clock.h>

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
//
void test_timer(void *paylod){
  static datetime_t datetime;
  time_getdatetime(&datetime);

  char buf[128];
  sprintf_(buf, "local time: %u/%02u/%02u %02u:%02u:%02u\r",
           datetime.year,
           datetime.month,
           datetime.day,
           (datetime.hour + 8) % 24,
           datetime.minute,
           datetime.second);

  tty_put_str_at_line(buf, 10, VGA_COLOR_RED | VGA_COLOR_GREEN );

}


int _kernel_main() {

  // char *alloc_test = (char *) kmalloc(64);
  // for(int i=0; i<20; ++i){
  //   alloc_test[i] = i;
  // }
  // for(int i=0; i<20; ++i){
  //   printf_("%u ", alloc_test[i]);
  // }
  //
  // kfree(alloc_test);
 
  /*test timer*/
  timer_run_second(1, test_timer, NULL, TIMER_MODE_PERIODIC);
  
  /*test keyboard*/
  struct kdb_keyinfo_pkt keyevent;
  while (1) {
      if (!kbd_recv_key(&keyevent)) {
          continue;
      }
      if ((keyevent.state & KBD_KEY_FPRESSED) && (keyevent.keycode & 0xff00) <= KEYPAD) {
          tty_put_char((char)(keyevent.keycode & 0x00ff));
          tty_sync_cursor();
      }
  }

  while (1) play_cxk_gif();

}


