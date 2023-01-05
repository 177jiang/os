#ifndef __jyos_proc_h_
#define __jyos_proc_h_

#include <syscall.h>

__SYSTEMCALL_0(void, yield)

__SYSTEMCALL_1(void, exit, int, status)

#endif

