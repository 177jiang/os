#include <arch/x86/interrupts.h>
#include <tty/tty.h>
#include <libc/stdio.h>
#include <libc/stdlib.h>
#include <process.h>
#include <spike.h>
#include <hal/apic.h>
#include <hal/cpu.h>
#include <mm/page.h>


static interrupt_function interrupt_vecotrs[INTERRUPTS_VECTOR_SIZE];

static interrupt_function   fallback = (interrupt_function) 0;

void intr_setvector(const unsigned int vector, interrupt_function fun){
    interrupt_vecotrs[vector] = fun;
}

void intr_unsetvector(const unsigned int vector, interrupt_function fun){
    if(interrupt_vecotrs[vector] == fun){
        interrupt_vecotrs[vector] = (interrupt_function) 0;
    }
}

void intr_set_fallback_handler(interrupt_function fun){
    fallback = fun;
}

void _msg_panic (const char *msg, isr_param* param){

    char buf[1024];
    sprintf_(buf, "INT %u: (%x) [%p: %p] %s",
            param->vector, param->err_code, param->cs, param->eip, msg);
    tty_set_theme(VGA_COLOR_RED, VGA_COLOR_BLACK);
    kprintf_(buf);
    spin();
}

extern x86_page_t *__kernel_pg_dir;

void _intr_handler(isr_param* param){

    __current->regs = *param;

    isr_param *tparam = &__current->regs;

    do{
        if(tparam->vector < 256){
            interrupt_function int_fun = interrupt_vecotrs[tparam->vector];
            if(int_fun){
                int_fun(param);
                break;
            }
        }
        if(fallback){
            fallback(tparam);
            break;
        }
        kprintf_error("INT %u: (%x) [%p: %p] Unknown",
                tparam->vector,
                tparam->err_code,
                tparam->cs,
                tparam->eip);
    }while(0);


    if (tparam->vector >= EX_INTERRUPT_BEGIN && tparam->vector != APIC_SPIV_IV) {
        apic_done_servicing();
    }

    return;
}


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

    kprintf_("%s", msg);

}



void _interrupts_handler(isr_param *param){
  if(param->vector == 0){
    isr0(param);
  }else if(param->vector == JYOS_SYS_PANIC){
      _panic_msg((const char *)param->edi);
      spin();
  }

}



