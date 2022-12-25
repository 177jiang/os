#include <arch/x86/interrupts.h>
#include <jyos/tty/tty.h>
#include <libc/stdio.h>
#include <libc/stdlib.h>

void isr0(isr_param *param){

    // play_cxk_gif();
    char str[64] = "";
    tty_put_str(itoa(param->eflags, str, 10));      tty_put_str("  eflags\n");
    tty_put_str(itoa(param->cs, str, 10));          tty_put_str("  cs\n");
    tty_put_str(itoa(param->eip, str, 10));         tty_put_str("  eip\n");
    tty_put_str(itoa(param->err_code, str, 10));    tty_put_str("  err_code\n");
    tty_put_str(itoa(param->vector, str, 10));      tty_put_str("  vector\n");
    tty_put_str(itoa(param->eax, str, 10));         tty_put_str("  eax\n");
    tty_put_str(itoa(param->ebx, str, 10));         tty_put_str("  ebx\n");
    tty_put_str(itoa(param->ecx, str, 10));         tty_put_str("  ecx\n");
    tty_put_str(itoa(param->edx, str, 10));         tty_put_str("  edx\n");
    tty_put_str(itoa(param->edi, str, 10));         tty_put_str("  edi\n");
    tty_put_str(itoa(param->esi, str, 10));         tty_put_str("  esi\n");
    tty_put_str(itoa(param->ebp, str, 10));         tty_put_str("  ebp\n");
    tty_put_str(itoa(param->ds, str, 10));          tty_put_str("  ds\n");
    tty_put_str(itoa(param->es, str, 10));          tty_put_str("  es\n");
    tty_put_str(itoa(param->fs, str, 10));          tty_put_str("  fs\n");

}

void _panic_msg(const char *msg){

    tty_set_theme(VGA_COLOR_RED, VGA_COLOR_BLACK);

    printf("%s", msg);

}

void panic (const char *msg, isr_param* param){
    char buf[1024];
    sprintf(buf, "INT %u: (%x) [%p: %p] %s",
            param->vector, param->err_code, param->cs, param->eip, msg);
    _panic_msg(msg);
    while(1);
}


void _interrupts_handler(isr_param *param){
  if(param->vector == 0){
    isr0(param);
  }else if(param->vector == JYOS_SYS_PANIC){
      _panic_msg((const char *)param->edi);
      while(1);
  }

}



