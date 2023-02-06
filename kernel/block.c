#include <block.h>

#include <libc/string.h>
#include <libc/stdio.h>
#include <libc/crc.h>

#include <mm/cake.h>
#include <mm/valloc.h>

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

void block_init(){

  lbd_pile     =
    cake_pile_create("block_dev", sizeof(struct block_dev), 1, 0);
  dev_registry = vcalloc(sizeof(struct block_dev*), MAX_DEV);
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
    error = __block_mount_partitions(hd_dev);
    if(error) break;

    return 1;
  }while(0);

  kprintf_error(
      "Failed to mount hd: %s[%s] (%x)\n",
      hd_dev->model,
      hd_dev->serial_num,
      -error
    );

  return 0;
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
    dev_registry[free_slot++] = dev;
    return 1;
}




