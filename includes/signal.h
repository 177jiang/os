#ifndef __jyos_signal_h_
#define __jyos_signal_h_

#include <syscall.h>

#define _SIG_MAX 16

#define _SIG_PENDING(bitmap, sig) ((bitmap) & (1 << (sig)))

#define _SIGSEGV        1
#define _SIGALRM        2
#define _SIGCHLD        3
#define _SIGCLD         _SIGCHLD
#define _SIGINT         4
#define _SIGKILL        5
#define _SIGSTOP        6
#define _SIGCONT        7
#define _SIGTERM        8

#define _SIG_USER       15

#define __SIGNAL(num) (1 << (num))
#define __SIGSET(bitmap, num) (bitmap = bitmap | __SIGNAL(num))
#define __SIGTEST(bitmap, num) (bitmap & __SIGNAL(num))
#define __SIGCLEAR(bitmap, num) ((bitmap) = (bitmap) & ~__SIGNAL(num))

#define _SIGNAL_UNMASKABLE (__SIGNAL(_SIGKILL) | __SIGNAL(_SIGSTOP))

#define _SIG_BLOCK 1
#define _SIG_UNBLOCK 2
#define _SIG_SETMASK 3

typedef unsigned int signal_t;

typedef void (*signal_handler) (int);

void *signal_dispatch();

__SYSTEMCALL_2(int, signal,
               int, signum,
               signal_handler, handler);

__SYSTEMCALL_1(int, sigpending,
               signal_t, *set);

__SYSTEMCALL_1(int, sigsuspend,
               const signal_t, *mask);

__SYSTEMCALL_3(int, sigprocmask,
               int, how,
               const signal_t, *set,
               signal_t, *old);


#endif










