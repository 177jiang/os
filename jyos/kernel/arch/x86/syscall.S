#define __ASM_S_
#include <syscall.h>

.section .data
 
    syscall_table:
        .dc.l 0
        .dc.l dup_proc
        .dc.l schedule
        .dc.l terminate_task
        .dc.l _syscall_sbrk
        .dc.l _syscall_brk

.global syscall_handler

.section .text
    syscall_handler:

    pushl %ebp
    movl %esp, %ebp

    addl  $0x8, %ebp
    movl (%ebp), %ebp

    movl 36(%ebp), %eax

    cmpl $__SYSCALL_MAX, %eax
    jb __proper_call
    neg  %eax
    jmp __end_call

__proper_call:

    pushl  12(%ebp)   //ebp
    pushl  16(%ebp)   //esi
    pushl  20(%ebp)   //edi
    pushl  24(%ebp)   //edx
    pushl  28(%ebp)   //ecx
    pushl  32(%ebp)   //ebx

    shll $2, %eax
    addl $syscall_table, %eax

    call (%eax)

    addl $24, %esp

__end_call:
    popl %ebp
    ret

