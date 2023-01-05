#ifndef __jyos_syscall_h_
#define __jyos_syscall_h_

#include <arch/x86/interrupts.h>

#define __SYSCALL_fork      0x1
#define __SYSCALL_yield     0x2
#define __SYSCALL_exit      0x3
#define __SYSCALL_sbrk      0x4
#define __SYSCALL_brk       0x5

#define __SYSCALL_MAX    0x100

#ifndef     __ASM_S_
void syscall_install();

static void *syscall(unsigned int callcode){

    asm volatile(
            "int %0"
            ::"i"(JYOS_SYS_CALL), "D"(callcode)
            :"eax"
    );
}


#define __DO_INT_33(ret_type, call_vector)          \
    int v;                                          \
    asm volatile(                                   \
            "int %1\n"                              \
            :"=a"(v)                                \
            :"i"(JYOS_SYS_CALL), "a"(call_vector)); \
    return (ret_type)v;                             \


#define __SYSTEMCALL_0(ret_type, name)              \
    ret_type name () {                              \
        __DO_INT_33(ret_type, __SYSCALL_##name)      \
    }

#define __SYSTEMCALL_1(ret_type, name, tp1, v1)     \
    ret_type name (tp1 v1) {                        \
        asm(""::"b"(v1));                           \
        __DO_INT_33(ret_type, __SYSCALL_##name);     \
    }

#define __SYSTEMCALL_2(ret_type, name, tp1, v1, tp2, v2)                    \
    ret_type name (tp1 v1, tp2 v2) {                                        \
        asm("\n"::"b"(v1), "c"(v2));                                        \
        __DO_INT_33(ret_type, __SYSCALL_##name);                             \
    }

#define __SYSTEMCALL_3(ret_type, name, tp1, v1, tp2, v2, tp3, v3)           \
    ret_type name (tp1 v1, tp2 v2, tp3 v3) {                                \
        asm("\n"::"b"(v1), "c"(v2), "d"(v3));                               \
        __DO_INT_33(ret_type, __SYSCALL_##name);                             \
    }

#define __SYSTEMCALL_4(ret_type, name, tp1, v1, tp2, v2, tp3, v3, tp4, v4)  \
    ret_type name (tp1 v1, tp2 v2, tp3 v3, tp4 v4) {                        \
        asm("\n"::"b"(v1), "c"(v2), "d"(v3), "D"(v4));                      \
        __DO_INT_33(ret_type, __SYSCALL_##name);                             \
    }






#endif

#endif
