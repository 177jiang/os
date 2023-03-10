#include <hal/ahci/hba.h>
#include <hal/ahci/scsi.h>
#include <hal/ahci/utils.h>
#include <hal/ahci/ahci.h>
#include <hal/ahci/sata.h>

#include <libc/string.h>
#include <libc/stdio.h>
#include <fs/fctrl.h>
#include <fs/foptions.h>

#include <spike.h>
#include <proc.h>
#include <junistd.h>

char test_poem[] = 
        "Being a cloud in the sky,"
        "On your heart lake I cast my figure."
        "You don't have to wonder,";



void __test_disk_io() {

    struct hba_port *port = ahci_get_port(0);

    char *buffer = valloc_dma(port->device->block_size);

    strcpy(buffer, test_poem);
    kprintf_live("WRITE: %s\n", buffer);
    int result;

    // 写入第一扇区 (LBA=0)
    result = port->device->ops.write_buffer(port->device, 0, buffer, port->device->block_size);


    if (!result) {
        kprintf_warn( "fail to write: %x\n", port->device->last_error);
    }

    memset(buffer, 0, port->device->block_size);

    // 读出我们刚刚写的内容！
    result =
      port->device->ops.read_buffer(port->device, 0, buffer, port->device->block_size);
    kprintf_warn("%x, %x\n", port->base[HBA_PxIS], port->base[HBA_PxTFD]);

    if (!result) {
        kprintf_error("fail to read: %x\n", port->device->last_error);
    } else {
        kprintf_hex(buffer, 512);
    }

    vfree_dma(buffer);

}

void __test_io(){

    int fd  = open("/dev/sda", 0);
    int tty = open("/dev/tty", F_DIRECT);

    if(fd < 0 || tty < 0){
        kprintf_error("fail to open (%d)\n", geterror());
        return ;
    }

    lseek(fd, 512, FSEEK_SET);
    write(fd, test_poem, sizeof(test_poem));
    lseek(fd, 512, FSEEK_SET);
    char read_out[256];
    read(fd, read_out, sizeof(read_out));
    close(fd);

    write(tty, read_out, strlen(read_out));
    close(tty);


    kprintf_live("%s\n",read_out);
}

