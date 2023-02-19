#ifndef __jyos_syscall_h_
#define __jyos_syscall_h_

#include <arch/x86/interrupts.h>

#define __SYSCALL_fork          1
#define __SYSCALL_yield         2
#define __SYSCALL_sbrk          3
#define __SYSCALL_brk           4
#define __SYSCALL_getpid        5
#define __SYSCALL_getppid       6
#define __SYSCALL_sleep         7
#define __SYSCALL__exit         8
#define __SYSCALL_wait          9
#define __SYSCALL_waitpid       10
#define __SYSCALL_sigreturn     11
#define __SYSCALL_sigprocmask   12
#define __SYSCALL_signal        13
#define __SYSCALL_pause         14
#define __SYSCALL_kill          15
#define __SYSCALL_alarm         16
#define __SYSCALL_sigpending    17
#define __SYSCALL_sigsuspend    18
#define __SYSCALL_open          19
#define __SYSCALL_close         20
#define __SYSCALL_read          21
#define __SYSCALL_write         22
#define __SYSCALL_readdir       23
#define __SYSCALL_mkdir         24
#define __SYSCALL_lseek         25
#define __SYSCALL_geterror      26
#define __SYSCALL_readlink      27
#define __SYSCALL_readlinkat    28
#define __SYSCALL_rmdir         29
#define __SYSCALL_unlink        30
#define __SYSCALL_unlinkat      31
#define __SYSCALL_link          32
#define __SYSCALL_fsync         33


#define __SYSCALL_MAX    0x100

#define __SYSCALL_MAX_PARAMETER (6 << 2)

#ifndef     __ASM_S_

#define   SYSCALL_ESTATUS(error)    -(!!(error))
void syscall_init();

#define __SYSCALL_INTERRUPTIBLE(code)               \
    asm("sti");                                     \
    { code };                                       \
    asm("cli");


static void *syscall(unsigned int callcode){

    asm volatile(
            "int %0"
            ::"i"(JYOS_SYS_CALL), "D"(callcode)
            :"eax"
    );
}

#define asmlinkage __attribute__((regparm(0)))

#define __PARAM_MAP1(t1, v1) t1 v1
#define __PARAM_MAP2(t1, v1, ...) t1 v1, __PARAM_MAP1(__VA_ARGS__)
#define __PARAM_MAP3(t1, v1, ...) t1 v1, __PARAM_MAP2(__VA_ARGS__)
#define __PARAM_MAP4(t1, v1, ...) t1 v1, __PARAM_MAP3(__VA_ARGS__)
#define __PARAM_MAP5(t1, v1, ...) t1 v1, __PARAM_MAP4(__VA_ARGS__)
#define __PARAM_MAP6(t1, v1, ...) t1 v1, __PARAM_MAP5(__VA_ARGS__)

#define __DEFINE_SYSTEMCALL_0(ret_type, name) \
    asmlinkage ret_type __do_##name()

#define __DEFINE_SYSTEMCALL_1(ret_type, name, t1, v1) \
    asmlinkage ret_type __do_##name(t1 v1)

#define __DEFINE_SYSTEMCALL_2(ret_type, name, t1, v1, t2, v2) \
    asmlinkage ret_type __do_##name(t1 v1, t2 v2)

#define __DEFINE_SYSTEMCALL_3(ret_type, name, t1, v1, t2, v2, t3, v3) \
    asmlinkage ret_type __do_##name(t1 v1, t2 v2, t3 v3)

#define __DEFINE_SYSTEMCALL_4(ret_type, name, t1, v1, t2, v2, t3, v3, t4, v4) \
    asmlinkage ret_type __do_##name(t1 v1, t2 v2, t3 v3, t4 v4)

#define __DO_INT_33(ret_type, call_vector)          \
    int v;                                          \
    asm volatile(                                   \
            "int %1\n"                              \
            :"=a"(v)                                \
            :"i"(JYOS_SYS_CALL), "a"(call_vector)); \
    return (ret_type)v;                             \


#define __SYSTEMCALL_0(ret_type, name)              \
    static ret_type name () {                              \
        __DO_INT_33(ret_type, __SYSCALL_##name)     \
    }

#define __SYSTEMCALL_1(ret_type, name, tp1, v1)     \
    static ret_type name (tp1 v1) {                        \
        asm(""::"b"(v1));                           \
        __DO_INT_33(ret_type, __SYSCALL_##name);    \
    }

#define __SYSTEMCALL_2(ret_type, name, tp1, v1, tp2, v2)                    \
    static ret_type name (tp1 v1, tp2 v2) {                                 \
        asm("\n"::"b"(v1), "c"(v2));                                        \
        __DO_INT_33(ret_type, __SYSCALL_##name);                            \
    }

#define __SYSTEMCALL_3(ret_type, name, tp1, v1, tp2, v2, tp3, v3)           \
    static ret_type name (tp1 v1, tp2 v2, tp3 v3) {                         \
        asm("\n"::"b"(v1), "c"(v2), "d"(v3));                               \
        __DO_INT_33(ret_type, __SYSCALL_##name);                            \
    }

#define __SYSTEMCALL_4(ret_type, name, tp1, v1, tp2, v2, tp3, v3, tp4, v4)  \
    static ret_type name (tp1 v1, tp2 v2, tp3 v3, tp4 v4) {                 \
        asm("\n"::"b"(v1), "c"(v2), "d"(v3), "D"(v4));                      \
        __DO_INT_33(ret_type, __SYSCALL_##name);                            \
    }


#endif

#endif
