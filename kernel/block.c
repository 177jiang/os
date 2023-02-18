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

int __block_mount_partitions(struct hba_device *hd_dev);

int __block_register(struct block_dev *dev);

int __block_read(struct v_file *file, void *buffer, size_t len);

int __block_write(struct v_file *file, void *buffer, size_t len);

void block_init(){

  lbd_pile     =
    cake_pile_create("block_dev", sizeof(struct block_dev), 1, 0);
  dev_registry = vcalloc(sizeof(struct block_dev*), MAX_DEV);
  free_slot    = 0;
}

void block_rootfs_create(){

    struct rootfs_node *dev       =  rootfs_toplevel_node("dev", 3);
    struct rootfs_node *dev_block =  rootfs_dir_node(dev, "block", 5);

    if(!dev_block){

        kprintf_error("fail to create rootfs node (block) \n");
        return ;
    }

    struct block_dev *bdev;
    struct rootfs_node *bnode;
    for(size_t i=0; i<MAX_DEV; ++i){

        if(!(bdev = dev_registry[i])){
            continue;
        }

        bnode =
          rootfs_dir_node(dev_block, bdev->devid, strlen(bdev->devid));
        bnode->fops.read    =  __block_read;
        bnode->fops.write   =  __block_write;
        bnode->data         =  i;
        bnode->inode->fsize =  bdev->hd_dev->max_lba;
    }
}

#define INVALID_BDEV(idx, bdev)     \
    (idx<0 || idx>MAX_DEV || !(bdev=dev_registry[idx]))

int __block_read(struct v_file *file, void *buffer, size_t len){
  
    int idx = (int)((struct rootfs_node*)(file->inode->data))->data;

    struct block_dev *bdev;

    if(INVALID_BDEV(idx, bdev)){
        return ENXIO;
    }

    size_t rd_size = 0, rd_block = 0;
    size_t bsize   = bdev->hd_dev->block_size;
    void *bbuf     = valloc(bsize);
    int error;

    while(rd_size < len){

        if(!bdev->hd_dev->ops.read_buffer
            (bdev->hd_dev, file->f_pos+rd_block, bbuf, bsize)
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

int __block_write(struct v_file *file, void *buffer, size_t len){
  
    int idx = (int)((struct rootfs_node*)(file->inode->data))->data;
    struct block_dev *bdev;

    if(INVALID_BDEV(idx, bdev)){
        return ENXIO;
    }

    size_t wr_size = 0, wr_block = 0;
    size_t bsize   = bdev->hd_dev->block_size;
    void *bbuf     = valloc(bsize);
    int error;

    while(wr_size < len){

        int ws = MIN(len - wr_size, bsize);
        memcpy(bbuf, buffer + wr_size, ws);
        if(!bdev->hd_dev->ops.write_buffer
            (bdev->hd_dev, file->f_pos + wr_block, bbuf, bsize)
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
  struct block_dev *dev = cake_piece_grub(lbd_pile);
  strncpy(dev->name, hd_dev->model, PARTITION_NAME_SIZE);
  dev->hd_dev   =  hd_dev;
  dev->base_lba =  0;
  dev->end_lba  =  hd_dev->max_lba;

  do{
    if(!__block_register(dev)){
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

int __block_mount_partitions(struct hba_device *hd_dev){

    int error    =  0 ;
    void *buffer =  valloc_dma(hd_dev->block_size);
    do{

      if(!hd_dev->ops.read_buffer(hd_dev, 1, buffer, hd_dev->block_size)){
          error = BLOCK_EREAD;
          break;
      }
      struct lpt_header *header = buffer;
      if(header->signature != LPT_SIG){
          error = BLOCK_ESIG;
          break;
      }
      if(header->crc != crc32b(buffer, sizeof(*header))){
          error = BLOCK_ECRC;
          break;
      }

      uint64_t lba = header->pt_start_lba;
      int j = 0;
      int count_per_sector = hd_dev->block_size / sizeof (struct lpt_entry);

      while(lba < header->pt_end_lba){
          if(!hd_dev->ops.read_buffer
              (hd_dev, lba, buffer, hd_dev->block_size)){
              error = BLOCK_EREAD;
              break;
          }

          struct lpt_entry *entry = buffer;
          for(int i=0; i<count_per_sector; ++i, ++j){
              struct block_dev *dev = cake_piece_grub(lbd_pile);
              strncpy(dev->name, entry->part_name, PARTITION_NAME_SIZE);
              dev->hd_dev   =  hd_dev;
              dev->base_lba =  entry->base_lba;
              dev->end_lba  =  entry->end_lba;
              __block_register(dev);
              if(j >= header->table_len){
                  error = BLOCK_ERROR;
                  break;
              }
          }
          if(error)break;
      }
      if(error) break;

    }while(0);

    vfree_dma(buffer);
    return -error;
}



int __block_register(struct block_dev *dev){

    if(free_slot >= MAX_DEV){
      return 0;
    }

    snprintf_(dev->devid, DEVID_NAME_SIZE, "bd%x", free_slot);
    dev_registry[free_slot++] = dev;
    return 1;
}




