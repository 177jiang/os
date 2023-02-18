#ifndef __jyos_fctl_h_
#define __jyos_fctl_h_

#include <fs/dirent.h>
#include <syscall.h>


__SYSTEMCALL_2(int, open,
               const char *, path,
               int, options);

__SYSTEMCALL_1(int, close,
               int, fd);

__SYSTEMCALL_1(int, mkdir,
               const char *, path);

__SYSTEMCALL_2(int, readdir,
               int, fd,
               struct dirent *, dent);

__SYSTEMCALL_3(int, lseek,
               int, fd,
               int, offset,
               int, options);

__SYSTEMCALL_3(int,     read,
               int,     fd,
               void *,  buffer,
               unsigned int, count);

__SYSTEMCALL_3(int,    write,
               int,    fd,
               void *, buffer,
               unsigned int, count);



#endif
