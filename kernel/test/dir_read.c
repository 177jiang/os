
#include <fs/dirent.h>
#include <fs/fctrl.h>
#include <junistd.h>
#include <libc/stdio.h>


void __test_readdir(){

    int fd = open("/dev/./../dev/.", 0);
    if(fd == -1){
        kprintf_error("fail to open !!! \n");
        return;
    }
    kprintf_live("succeed to open !!! \n");

    char path[129];
    int len = realpathat(fd, path, 128);
    if(len < 0){
        kprintf_error("fail to read !!! \n");
    }else{
        path[len] = 0;
        kprintf_warn("%s\n", path);
    }

    struct dirent dt = { .d_offset = 0 };

    while(!(readdir(fd, &dt))) {

        kprintf_warn("%s\n", dt.d_name);
    }


    close(fd);
    return;
}
