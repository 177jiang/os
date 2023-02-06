#include <hal/ahci/hba.h>
#include <hal/ahci/sata.h>

#include <mm/valloc.h>
#include <mm/vmm.h>

#include <spike.h>


int __sata_buffer_io(
    struct hba_device* dev,
    uint64_t lba,
    void  *buffer,
    uint32_t size,
    int write){

    assert_msg(((uintptr_t)buffer & 0x3) == 0, "HBA: Bad buffer alignment");

    struct hba_cmd_header *cmdh;
    struct hba_cmd_table  *cmdt;

    struct hba_port *port = dev->port;
    int slot = hba_prepare_cmd(port, &cmdt, &cmdh, buffer, size);

    int bitmask = 1 << slot;

    // 确保端口是空闲的
    wait_until(!(port->base[HBA_PxTFD] & (HBA_TFD_BSY | HBA_TFD_DRQ)));


    port->base[HBA_PxIS] = 0;

    cmdh->options |= HBA_CMDH_WRITE * !!(write);

    uint16_t count = ICEIL(size, port->device->block_size);
    struct sata_reg_fis* fis = cmdt->CFIS;

    if ((port->device->signature & HBA_DEV_FEXTLBA)) {
        // 如果该设备支持48位LBA寻址
        sata_create_fis(
          fis, write ? ATA_WRITE_DMA_EXT : ATA_READ_DMA_EXT, lba, count);
    } else {
        sata_create_fis(fis, write ? ATA_WRITE_DMA : ATA_READ_DMA, lba, count);
    }

    fis->dev = (1 << 6);

    int retries = 0;

    while (retries < MAX_RETRY) {
        port->base[HBA_PxCI] = bitmask;

        wait_until(!(port->base[HBA_PxCI] & bitmask));

        if ((port->base[HBA_PxTFD] & HBA_TFD_ERR)) {
            // 有错误
            sata_read_error(port);
            retries++;
        } else {
            vfree_dma(cmdt);
            return 1;
        }
    }

fail:
    vfree_dma(cmdt);
    return 0;
}



int sata_read_buffer(
    struct hba_device* dev,
    uint64_t lba,
    void  *buffer,
    uint32_t size) {

    return __sata_buffer_io(dev, lba, buffer, size, 0);
}

int sata_write_buffer(
    struct hba_device *dev,
    uint64_t lba,
    void *buffer,
    uint32_t size){
    
    return __sata_buffer_io(dev, lba, buffer, size, 1);

}

void sata_read_error(struct hba_port *port){

  uint32_t tfd              =  port->base[HBA_PxTFD];
  port->device->last_error  =  (tfd >> 8) & 0xFF;
  port->device->last_status =  tfd & 0xFF;

}
