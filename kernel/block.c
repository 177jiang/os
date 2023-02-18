#include <block.h>
#include <spike.h>

#include <libc/string.h>
#include <libc/stdio.h>
#include <libc/crc.h>

#include <mm/cake.h>
#include <mm/valloc.h>

#include <fs/rootfs.h>

#define  BLOCK_EREAD    1
#define  BLOCK_ESIG     2
#define  BLOCK_ECRC     3
#define  BLOCK_EFULL    4

#define  BLOCK_ERROR    7

#define  MAX_DEV        32

struct cake_pile *lbd_pile;
struct block_dev **dev_registry;
int free_slot;

int __block_register(struct block_dev *dev);

void block_init(){

  lbd_pile     =
    cake_pile_create("block_dev", sizeof(struct block_dev), 1, 0);
  dev_registry = vcalloc(sizeof(struct block_dev*), MAX_DEV);
  free_slot    = 0;
}

#define INVALID_BDEV(idx, bdev)     \
    (idx<0 || idx>MAX_DEV || !(bdev=dev_registry[idx]))

int __block_read(struct device *dev,
    void *buffer, size_t offset, size_t len){

    struct block_dev *bdev = dev->underlay;

    size_t rd_size = 0, rd_block = 0;
    size_t bsize   = bdev->hd_dev->block_size;
    void *bbuf     = valloc(bsize);
    int error;

    while(rd_size < len){

        if(!bdev->hd_dev->ops.read_buffer
            (bdev->hd_dev, offset + rd_block, bbuf, bsize)
          ){
            error = ENXIO;
            goto __error;
        }
        int rs = MIN(len - rd_size, bsize);
        memcpy(buffer + rd_size, bbuf, rs);
        rd_size += rs;
        ++rd_block;
    }

    vfree(bbuf);
    return rd_block;

__error:
    vfree(bbuf);
    return error;
}

int __block_write(struct device *dev,
    void *buffer, size_t offset, size_t len){
  
    struct block_dev *bdev = dev->underlay;

    size_t wr_size = 0, wr_block = 0;
    size_t bsize   = bdev->hd_dev->block_size;
    void *bbuf     = valloc(bsize);
    int error;

    while(wr_size < len){

        int ws = MIN(len - wr_size, bsize);
        memcpy(bbuf, buffer + wr_size, ws);
        if(!bdev->hd_dev->ops.write_buffer
            (bdev->hd_dev, offset + wr_block, bbuf, bsize)
          ){
            error = ENXIO;
            goto __error;
        }
        wr_size += ws;
        ++wr_block;
    }

    vfree(bbuf);
    return wr_block;

__error:
    vfree(bbuf);
    return error;
}



int block_mount_disk(struct hba_device *hd_dev){

  int error = 0;
  struct block_dev *bdev = cake_piece_grub(lbd_pile);
  strncpy(bdev->name, hd_dev->model, PARTITION_NAME_SIZE);
  bdev->hd_dev   =  hd_dev;
  bdev->base_lba =  0;
  bdev->end_lba  =  hd_dev->max_lba;

  do{
    if(!__block_register(bdev)){
      error = BLOCK_EFULL;
      break;
    }

    return error;
  }while(0);

  kprintf_error(
      "Failed to mount hd: %s[%s] (%x)\n",
      hd_dev->model,
      hd_dev->serial_num,
      -error
    );

  return error;
}

int __block_register(struct block_dev *bdev){

    if(free_slot >= MAX_DEV){
      return 0;
    }

    struct device *dev =
      device_add(NULL, bdev, "sd%c", 'a' + free_slot);

    dev->read  = __block_read;
    dev->write = __block_write;

    bdev->dev  = dev;

    dev_registry[free_slot++] = bdev;
    return 1;
}




