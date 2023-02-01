
#include <hal/ahci/hba.h>
#include <hal/ahci/utils.h>
#include <libc/string.h>>



#define IDDEV_OFFMAXLBA 60
#define IDDEV_OFFMAXLBA_EXT 230
#define IDDEV_OFFLSECSIZE 117
#define IDDEV_OFFWWN 108
#define IDDEV_OFFSERIALNUM 10
#define IDDEV_OFFMODELNUM 27
#define IDDEV_OFFADDSUPPORT 69
#define IDDEV_OFFALIGN 209
#define IDDEV_OFFLPP 106
#define IDDEV_OFFCAPABILITIES 49


void ahci_parse_str(char *str, uint16_t *start, int size_word){

  int j = 0;
  for(size_t i=0; i<size_word; ++i){

      str[j++]      =  (start[i] >> 8) & 0xFF;
      str[j++]      =  (start[i] & 0xFF);
  }
  str[--j] = '\0';
}

void ahci_parse_dev_info(struct hba_device *dev, uint16_t *data){


    dev->max_lba = *((uint32_t*)(data + IDDEV_OFFMAXLBA));
    dev->block_size = *((uint32_t*)(data + IDDEV_OFFLSECSIZE));
    dev->cbd_size = (*data & 0x3) ? 16 : 12;
    dev->wwn = *(uint64_t*)(data + IDDEV_OFFWWN);
    dev->block_per_sec = 1 << (*(data + IDDEV_OFFLPP) & 0xf);
    dev->alignment_offset = *(data + IDDEV_OFFALIGN) & 0x3fff;
    dev->capabilities = *((uint32_t*)(data + IDDEV_OFFCAPABILITIES));

    if (!dev->block_size) {
        dev->block_size = 512;
    }

    if ((*(data + IDDEV_OFFADDSUPPORT) & 0x8)) {
        dev->max_lba = *((uint64_t*)(data + IDDEV_OFFMAXLBA_EXT));
        dev->signature |= HBA_DEV_FEXTLBA;
    }

    ahci_parse_str(&dev->serial_num, data + IDDEV_OFFSERIALNUM, 10);
    ahci_parse_str(&dev->model, data + IDDEV_OFFMODELNUM, 20);

}

