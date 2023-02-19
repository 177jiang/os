#define __ASM_S_
#include <syscall.h>
#include <arch/x86/interrupts.h>

.section .data
 
    syscall_table:
        .long 0
        .long __do_fork
        .long __do_yield
        .long __do_sbrk
        .long __do_brk
        .long __do_getpid
        .long __do_getppid
        .long __do_sleep
        .long __do__exit
        .long __do_wait
        .long __do_waitpid
        .long __do_sigreturn
        .long __do_sigprocmask
        .long __do_signal
        .long __do_pause
        .long __do_kill                 /* pause 15 */
        .long __do_alarm
        .long __do_sigpending
        .long __do_sigsuspend
        .long __do_open
        .long __do_close
        .long __do_read
        .long __do_write
        .long __do_readdir
        .long __do_mkdir
        .long __do_lseek                  /* 25 */
        .long __do_geterror
        .long __do_readlink
        .long __do_readlinkat
        .long __do_rmdir
        .long __do_unlink
        .long __do_unlinkat

    __pos:
        .rept __SYSCALL_MAX - (__pos - syscall_table)/4
            .long 0
        .endr

.global syscall_handler

.section .text
    syscall_handler:

    pushl %ebp

    movl 8(%esp), %ebp

    movl REGS_EAX(%ebp), %eax

    cmpl $__SYSCALL_MAX, %eax
    jae __bad_call

    shll $2, %eax
    addl $syscall_table, %eax
    cmpl $0, (%eax)
    jne __proper_call

__bad_call:
    neg  %eax
    jmp __end_call

__proper_call:

    pushl  REGS_EBP(%ebp)   //ebp
    pushl  REGS_ESI(%ebp)   //esi
    pushl  REGS_EDI(%ebp)   //edi
    pushl  REGS_EDX(%ebp)   //edx
    pushl  REGS_ECX(%ebp)   //ecx
    pushl  REGS_EBX(%ebp)   //ebx

    call (%eax)

    movl %eax, REGS_EAX(%ebp)

    addl $__SYSCALL_MAX_PARAMETER, %esp

__end_call:
    popl %ebp
    ret

