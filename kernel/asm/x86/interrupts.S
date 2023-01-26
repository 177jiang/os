
#define  __ASM_S_

#include <arch/x86/interrupts.h>
#include <syscall.h>
#include <constant.h>
#include <process.h>

.macro isr_template vector, no_error_code=1
    .global _asm_isr\vector
    .type _asm_isr\vector, @function
    _asm_isr\vector:
        .if \no_error_code
            pushl $0x0
        .endif
        pushl $\vector
        jmp _interrupt_wrapper
.endm

.section .bss
    .align 16
    t_tmp_task:
        .skip 128
    tmp_stack:

.section .text

  isr_template      FAULT_DIVISION_ERROR
  isr_template      FAULT_GENERAL_PROTECTION, no_error_code=0
  isr_template      FAULT_PAGE_FAULT, no_error_code=0
  isr_template      FAULT_STACK_SEG_FAULT, no_error_code=0

  isr_template      JYOS_SYS_PANIC
  isr_template      JYOS_SYS_CALL

  isr_template      APIC_ERROR_IV
  isr_template      APIC_LINT0_IV                                /* ... */  /*old_esp*/
  isr_template      APIC_TIMER_IV                                /* eflag */
  isr_template      APIC_SPIV_IV                                 /* cs */
  isr_template      RTC_TIMER_IV                                 /* eip */
  isr_template      PC_KBD_IV                                    /* err_code */
                                                                 /* vector */
  _interrupt_wrapper:                                            /* esp */

    pushl %esp                                                   /* ebx */
                                                                 /* ecx */
    pushl %eax                                                   /* edx */
    pushl %ebx                                                   /* edi */
    pushl %ecx                                                   /* esi */
    pushl %edx                                                   /* ebp */
    pushl %edi                                                   /* ds */
    pushl %esi                                                   /* es */
    pushl %ebp                                                   /* fs         :esp_old */
    push  %ds                                                    /*  . */
    push  %es                                                    /*  . */
    push  %fs                                                    /*  . */
    push  %gs                                                    /* esp_pld    :esp_new */
                                                                 /* eax */                                   
__check_ring:
    cld
    movl REGS_CS(%esp), %eax
    andl $0x3,          %eax

    jz   __after_change_seg

    movl $K_DATA_SEG, %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    /* save user stack pointer */
__save_pointer:
    movl (__current),       %eax
    movl REGS_UESP(%esp),   %ebx
    movl %ebx,              TASK_OFF_USTACK(%eax)

__after_change_seg:
    movl %esp, %eax
    andl $0xFFFFFFF0, %esp
    subl $16, %esp
    movl %eax, (%esp)    /*save  param -> (esp) */

    /* call _interrupts_handler */
    call _intr_handler

    movl (%esp), %eax   /* param -> eax */

.global soft_iret
soft_iret:

    movl %eax, %esp    /* eax(param) -> esp */

    popl %gs
    popl %fs
    popl %es
    popl %ds
    popl %ebp
    popl %esi
    popl %edi
    popl %edx
    popl %ecx
    popl %ebx
    popl %eax

    popl %esp

    addl $8, %esp  //  at eip

    /*process tss*/
    pushl %eax            /* esp at (error_code) */

    movl 8(%esp), %eax   /* cs -> eax */
    andl $3,      %eax
    setnz         %al
    shll $3,      %eax
    addl $16,     %eax
    addl %esp,    %eax
    movl %eax,    (_tss + 4)

    popl %eax

    iret

.global __do_switch_to
__do_switch_to:

    popl %ebx   /*new_task -> ebx*/

    movl __current, %eax
    movl TASK_OFF_PAGETABLE(%eax), %ecx
    movl TASK_OFF_PAGETABLE(%ebx), %eax

    cmpl %ecx, %eax

    jz  __after_change_pgtable

    movl %eax, %cr3

__after_change_pgtable:

    movl %ebx, __current
    movl $tmp_stack, %esp /* use a temp task */

    call signal_dispatch

    test %eax, %eax
    jz __after_process_signal

    jmp handle_signal

 __after_process_signal:

    movl __current, %eax /* param -> eax */

    jmp soft_iret


handle_signal:

    pushl   $U_DATA_SEG         /* ss */
    pushl   %eax                /* temp_eip(sig_struct) setup in signal_dispatch */
    pushl   REGS_EFLAGS(%eax)   /* eflags */
    pushl   $U_CODE_SEG         /* cs */
    pushl   $sig_wrapper        /* eip */

    movw  $U_DATA_SEG, %cx
    movw  %cx,         %gs
    movw  %cx,         %fs
    movw  %cx,         %es
    movw  %cx,         %ds

    iret

.section .user_text
sig_wrapper:

    movl %esp, %eax             /* esp is sig_struct */
    andl $0xFFFFFFF0, %esp      /* align */
    subl $16, %esp

    pushl %eax                  /* save sig_struct */
    pushl SIG_OFF_NUM(%eax)     /* pushl num  for handler*/

    call  SIG_OFF_HANDLER(%eax)  /* call sig handler */

    movl $__SYSCALL_sigreturn, %eax
    movl 4(%esp),              %ebx

    int $JYOS_SYS_CALL










