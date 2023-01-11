#ifndef __jyos_signal_h_
#define __jyos_signal_h_

#include <syscall.h>

#define _SIG_MAX   8

#define _SIG_PENGIND(map, sig)   ((map) & (1 << sig))

#define _SIGSEGV 0
#define _SIGALRM 1
#define _SIGCHLD 2
#define _SIGCLD SIGCHLD
#define _SIGINT 3
#define _SIGKILL 4
#define _SIGSTOP 5
#define _SIGCONT 6

#define _SIGNAL_UNMASKABLE      ((1 << _SIGKILL) | (1 << _SIGSTOP))
#define _SIGNAL(s)              (1 << (s));
#define _SIGNAL_SET(sig, s)     ((sig) = (sig) | (_SIGNAL(s)))

#define _SIG_BLOCK 1
#define _SIG_UNBLOCK 2
#define _SIG_SETMASK 3

typedef unsigned int signal_t;

typedef void (*signal_handler) (int);

void *signal_dispatch();

__SYSTEMCALL_2(int, signal, int, signum, signal_handler, handler);

#endif
