#ifndef __jyos_proc_h_
#define __jyos_proc_h_

#include <syscall.h>
#include <process.h>

__SYSTEMCALL_0(void, yield);

__SYSTEMCALL_1(pid_t, wait, int*, state);

__SYSTEMCALL_3(pid_t, waitpid, pid_t, pid, int*, state, int, options);

__SYSTEMCALL_0(int, geterror);

#endif

