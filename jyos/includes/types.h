#ifndef __jyos_types_h_
#define __jyos_types_h_

#include <stdint.h>

#define TASKTERM 0x10000
#define TASKSTOP 0x20000

#define WNOHANG 1

#define WUNTRACED 2

#define WEXITSTATUS(wstatus) ((wstatus & 0xFFFF))

#define WIFSTOPPED(wstatus)  ((wstatus & TASKSTOP))

#define WIFEXITED(wstatus)             \
    ((wstatus & TASKTERM) && ((short)WEXITSTATUS(wstatus) >= 0))



typedef int32_t pid;


#endif

