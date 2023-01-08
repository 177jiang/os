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
#include <proc.h>
#include <sched.h>
#include <junistd.h>

extern uint8_t __kernel_start;

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

  tty_put_str_at_line(buf, 0, VGA_COLOR_RED | VGA_COLOR_GREEN );

}

extern struct scheduler sched_ctx;

int _kernel_main() {

  // if(!fork()){
  //   printf_error("This is first fork\n");
  //   sleep(1);
  //   printf_error("After sleep now first fork exit\n");
  //   _exit(7);
  // }
  // int state ;
  // pid_t pid = wait(&state);
  // printf_error("Parent: chiled (%d) exit whith code(%d)\n", pid, state);

  for(int i=0; i<10; ++i){
    pid_t pid = fork();
    if(!pid){
      while(1){
          if(i==9)i = *(uint32_t*)0xDEADC0DE;
          if(i == 7){
            printf_warn("This is a living task(%d)\n", i);
            yield();
          }else{
            sleep(1);
            _exit(0);
          }
      }
    }
    printf_error("create task %d\n",pid);
  }

   char buf[64];
   cpu_get_brand(buf);
   printf_("CPU: %s\n\n", buf);
   printf_("------------------------- Initialization end !!! ----------------\n");

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

  play_cxk_gif();
  while(1);
}


