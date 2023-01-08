#ifndef __jyos_unistd_h_
#define __jyos_unistd_h_

#include <syscall.h>
#include <process.h>

__SYSTEMCALL_0(pid_t, fork)

__SYSTEMCALL_1(int, sbrk, void*, addr)

__SYSTEMCALL_1(void*, brk, size_t, size)

__SYSTEMCALL_0(pid_t, getpid)

__SYSTEMCALL_0(pid_t, getppid)

__SYSTEMCALL_1(void, _exit, int, status)

__SYSTEMCALL_1(unsigned int, sleep, unsigned int, seconds)

__SYSTEMCALL_1(pid_t, wait, int *, status);


#endif
