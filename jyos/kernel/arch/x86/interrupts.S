
#define  __INTERRUPTS_S_
#include <arch/x86/interrupts.h>

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



.section .text

  isr_template      FAULT_DIVISION_ERROR
  isr_template      FAULT_GENERAL_PROTECTION, no_error_code=0
  isr_template      FAULT_PAGE_FAULT, no_error_code=0

  isr_template      JYOS_SYS_PANIC

  isr_template      APIC_ERROR_IV
  isr_template      APIC_LINT0_IV
  isr_template      APIC_TIMER_IV
  isr_template      APIC_SPIV_IV
  isr_template      RTC_TIMER_IV
  isr_template      PC_KBD_IV

  _interrupt_wrapper:
  
    pushl %eax
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %edi
    pushl %esi
    pushl %ebp
    push  %ds
    push  %es
    push  %fs

    movl %esp, %eax
    andl $0xFFFFFFF0, %esp
    subl $16, %esp
    movl %eax, (%esp)

    movl $0x10, %edx
    movw %dx, %ds
    movw %dx, %es
    movw %dx, %fs

    /* call _interrupts_handler */
    call _intr_handler

    pop %eax
    movl %eax, %esp

    pop %fs
    pop %es
    pop %ds
    pop %ebp
    pop %esi
    pop %edi
    pop %edx
    pop %ecx
    pop %ebx
    pop %eax

    addl $8, %esp

    iret

/* ... */
/* eflag */
/* cs */
/* eip */
/* err_code */
/* vector */
/* eax */
/* ebx */
/* ecx */
/* edx */
/* edi */
/* esi */
/* ebp */
/* ds */
/* es */
/* fs         :esp_old */
/*  . */
/*  . */
/*  . */
/* esp_pld    :esp_new */
