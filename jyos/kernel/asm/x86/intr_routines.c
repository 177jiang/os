#include <arch/x86/interrupts.h>
#include <tty/tty.h>
#include <spike.h>
#include <hal/cpu.h>
#include <libc/stdio.h>
#include <process.h>

#include <hal/apic.h>

extern void intr_routine_page_fault(const isr_param *param);

void _intr_print_msg(const char *msg, const isr_param *param){

    char buf[1024];

    sprintf_(buf, "INT %u: (%x) [%p: %p] %s",
            param->vector, param->err_code, param->cs, param->eip, msg);

    printf_error(buf);

    spin();

}
void intr_routine_divide_zero(const isr_param *param){
    _intr_print_msg("Divide by zero !", param);
}

void intr_routine_general_protection(const isr_param *param){

    printf_error("Expected: %p\n", __current->regs.esp);
    printf_error("Task: %d\n", __current->pid);
    _intr_print_msg("General Protection !", param);

}


void intr_routine_sys_panic(const isr_param *param){
    _intr_print_msg((const char *)(param->edi), param);
}

void intr_routine_fallback(const isr_param *param){
    _intr_print_msg("Unknow Interrupt !", param);
}

void intr_routine_apic_spi(const isr_param *param){

}
void intr_routine_apic_error(const isr_param *param){

    uint32_t error_reg = apic_read_reg(APIC_ESR);
    char buf[32];
    sprintf_(buf, "APIC error, ESR=0x%x", error_reg);

    _intr_print_msg(buf, param);
}


void intr_routine_init(){

    intr_setvector(FAULT_DIVISION_ERROR,        intr_routine_divide_zero);
    intr_setvector(FAULT_GENERAL_PROTECTION,    intr_routine_general_protection);
    intr_setvector(FAULT_PAGE_FAULT,            intr_routine_page_fault);
    intr_setvector(FAULT_STACK_SEG_FAULT,       intr_routine_page_fault);

    intr_setvector(JYOS_SYS_PANIC,              intr_routine_sys_panic);
    intr_setvector(APIC_SPIV_IV,                intr_routine_apic_spi);
    intr_setvector(APIC_ERROR_IV,               intr_routine_apic_error);

    intr_set_fallback_handler(intr_routine_fallback);

}


