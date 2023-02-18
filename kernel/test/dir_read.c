
#include <fs/dirent.h>
#include <fs/fctrl.h>
#include <libc/stdio.h>


void __test_readdir(){

    int fd = open("/dev/block", 0);

    if(fd == -1){

        kprintf_error("fail to open !!! \n");
        return;
    }
    kprintf_live("succeed to open !!! \n");

    struct dirent dt = { .d_offset = 0 };

    while(!(readdir(fd, &dt))) {

        kprintf_warn("%s\n", dt.d_name);
    }

    close(fd);

    return;
}
