#ifndef __jyos_fctl_h_
#define __jyos_fctl_h_

#include <dirent.h>
#include <syscall.h>


__SYSTEMCALL_2(int, open,
               const char *, path,
               int, options);

__SYSTEMCALL_1(int, close,
               int, fd);

__SYSTEMCALL_1(int, mkdir,
               const cahr *, path);

__SYSTEMCALL_2(int, readdir,
               int, fd,
               struct dirent *, dent);




#endif
