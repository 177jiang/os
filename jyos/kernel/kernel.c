#include <arch/x86/boot/multiboot.h>
#include <arch/x86/gdt.h>
#include <arch/x86/idt.h>

#include <mm/kalloc.h>
#include <hal/io.h>
#include <hal/cpu.h>
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
#include <junistd.h>
#include <types.h>
#include <sched.h>
#include <spike.h>

extern uint8_t __kernel_start;

extern struct scheduler sched_ctx;

extern uint8_t __init_hhk_end;

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

void __USER_SPACE__ test_signal_handler(int signum){
  printf_error("Signal [%d]  received at task %d!!!! \n", signum, getpid());
}
void __USER_SPACE__ test_signal_kill(int signum){
  printf_error("Kill test at task %d!!!! \n",  getpid());
}

void test(int *a){
  *a = 5;
  printf_error("%x\n", a);
}

void __USER_SPACE__ sigsegv_handler(int signum) {
    pid_t pid = getpid();
    printf_warn("SIGSEGV received on process %d\n", pid);
    _exit(signum);
}

int __USER_SPACE__ _kernel_main() {

  // int status = 0;
  //
  // if(!fork()){
  //   printf_warn("This is a test for wait\n");
  //   sleep(3);
  //   _exit(3);
  // }
  //
  //
  // pid_t pid = 0;
  // for(int i=0; i<7; ++i){
  //   if(!(pid=fork())){
  //     while(1){
  //       signal(_SIG_USER, test_signal_kill);
  //       sleep(1);
  //       printf_live("%d\n", getpid());
  //       if(i !=6 )_exit(i);
  //     }
  //   }
  // }
  //
  //
  // signal(_SIGCHLD, test_signal_handler);
  // state = 0;
  // int a = waitpid(-1, &state, 0);
  //
  // printf_error("task %d exit with code %d\n", a, WEXITSTATUS(state));
  // kill(pid, _SIG_USER);
  //
  // state = 0;
  // for(int i=0; i<5; ++i){
  //   pid = waitpid(-1, &state, 0);
  //   printf_error("task %d exit with code %d\n", pid, WEXITSTATUS(state));
  // }
 // 
 //    pid_t p = 0;
 //
 //    if (!fork()) {
 //        printf_live("Test no hang!\n");
 //        sleep(6);
 //        _exit(0);
 //    }
 //
 //    waitpid(-1, &status, WNOHANG);
 //
 //    for (size_t i = 0; i < 5; i++) {
 //        pid_t pid = 0;
 //        if (!(pid = fork())) {
 //            signal(_SIGSEGV, sigsegv_handler);
 //            sleep(i);
 //
 //            tty_put_char('0' + i);
 //            tty_put_char('\n');
 //            _exit(0);
 //        }
 //        printf_warn("Forked %d\n", pid);
 //    }
 //
 //    while ((p = wait(&status)) >= 0) {
 //        short code = WEXITSTATUS(status);
 //        if (WIFSIGNALED(status)) {
 //            printf_error("Process %d terminated by signal, exit_code: %d\n",
 //                    p, code);
 //        } else if (WIFEXITED(status)) {
 //            printf_error("Process %d exited with code %d\n", p, code);
 //        } else {
 //            printf_error("Process %d aborted with code %d\n", p, code);
 //        }
 //    }

    char buf[64];

    printf_("Hello processes!\n");

    cpu_get_brand(buf);
    printf_("CPU: %s\n\n", buf);

  /*now we just for testing */
  /* just  the kernel code and data could be WR by user, and don't include heap */
  /* so if we call a function that uses a addr in heap_space , it will cause a page fault */
   /* such as timer queue */
  // timer_run_second(1, test_timer, NULL, TIMER_MODE_PERIODIC);

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

  spin();

}


