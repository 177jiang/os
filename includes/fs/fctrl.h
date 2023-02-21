#ifndef __jyos_fctl_h_
#define __jyos_fctl_h_

#include <fs/dirent.h>
#include <syscall.h>
#include <stddef.h>


__SYSTEMCALL_2(int, readdir,
               int, fd,
               struct dirent *, dent);

__SYSTEMCALL_4(int, readlinkat,
               int, dirfd,
               const char *, path,
               char *, buf,
               size_t, size);

__SYSTEMCALL_2(int, unlinkat,
               int, fd,
               const char *, path);

__SYSTEMCALL_3(int, realpathat,
               int, fd,
               char *, path,
               size_t, size);
#endif
