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

__SYSTEMCALL_0(int, pause)

__SYSTEMCALL_2(int, kill, pid_t, pid, int, sig)

__SYSTEMCALL_1(unsigned int, alarm, unsigned int, seconds)


#endif
