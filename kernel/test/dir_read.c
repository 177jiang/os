
#include <fs/dirent.h>
#include <fs/fctrl.h>
#include <libc/stdio.h>


void __test_readdir(){

    int fd = open("/bus/../..", 0);

    if(fd == -1){

        kprintf_error("fail to open !!! \n");
        return;
    }
    kprintf_live("succeed to open !!! \n");
    close(fd);

    return;
}
