#ifndef __jyos_types_h_
#define __jyos_types_h_

#include <stdint.h>

#define TEXITTERM 0x100
#define TEXITSTOP 0x200
#define TEXITSIG  0x400

#define TEXITNUM(flag, code) (flag | (code & 0xff))

#define WNOHANG 1

#define WUNTRACED 2

#define WEXITSTATUS(wstatus) ((wstatus & 0xFFFF))

#define WIFSTOPPED(wstatus)  ((wstatus & TEXITSTOP))

#define WIFEXITED(wstatus)             \
    ((wstatus & TEXITTERM) && ((short)WEXITSTATUS(wstatus) >= 0))

#define WIFSIGNALED(wstatus) ((wstatus & TEXITSIG))

#define is_number(c)    ((c)>='0' && (c)<='9')

typedef int32_t pid;


#endif

