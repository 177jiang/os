
#include <hal/ahci/hba.h>
#include <hal/ahci/utils.h>
#include <libc/string.h>>



#define IDDEV_MAXLBA_OFFSET       60
#define IDDEV_LSECSIZE_OFFSET     117
#define IDDEV_WWN_OFFSET          108
#define IDDEV_SERIALNUM_OFFSET    10
#define IDDEV_MODELNUM_OFFSET     27


void ahci_parse_str(char *str, uint16_t *start, int size_word){

  int j = 0;
  for(size_t i=0; i<size_word; ++i){

      str[j++]      =  (start[i] >> 8) & 0xFF;
      str[j++]      =  (start[i] & 0xFF);
  }
  str[--j] = '\0';
}

void ahci_parse_dev_info(struct hba_device *dev, uint16_t *data){


    dev->max_lba    =  *((uint32_t*)(data + IDDEV_MAXLBA_OFFSET));
    dev->block_size =  *((uint32_t*)(data + IDDEV_LSECSIZE_OFFSET));
    dev->cbd_size   =  (*data & 0x3) ? 12 : 16;

    memcpy(&dev->wwn, (uint8_t*)(data + IDDEV_WWN_OFFSET), 8);

    if (!dev->block_size) {
        dev->block_size = 512;
    }
    ahci_parse_str(&dev->serial_num, data + IDDEV_SERIALNUM_OFFSET, 10);
    ahci_parse_str(&dev->model, data + IDDEV_MODELNUM_OFFSET, 20);

}

