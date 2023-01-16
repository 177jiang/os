#define __ASM_S_
#include <constant.h>

.section .text

  .global hhk_entry_
  
  hhk_entry_:

; /* set gdt */
    subl $0x10, %esp

    call _init_gdt
    movl $_gdt, 2(%esp)
    movw _gdt_limit, %ax
    movw %ax, (%esp)
    lgdt (%esp)


; /* set idt */
    call _init_idt
    movl $_idt, 2(%esp)
    movw _idt_limit, %ax
    movw %ax, (%esp)
    lidt (%esp)
    
    addl $0x06, %esp


/* set register (r0_data) */
    movw $0x10, %cx
    movw %cx, %es
    movw %cx, %ds
    movw %cx, %fs
    movw %cx, %ds
    movw %cx, %ss
    

/* protect mode */
/* r0_code */
    ljmp $0x08, $___start_kernel
    jmp l_ /* test */

   ___start_kernel:

      movw $TSS_SEG, %ax
      ltr %ax

      movl $mb_info, _init_mb_info

      call _kernel_pre_init
      call _kernel_init


    cli
     l_:
       hlt
       jmp l_
