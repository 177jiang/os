
#ifndef __jyos_sata_h_
#define __jyos_sata_h_


#include <stdint.h>

#define SATA_REG_FIS_D2H                0x34
#define SATA_REG_FIS_H2D                0x27
#define SATA_REG_FIS_COMMAND            0x80
#define SATA_LBA_COMPONENT(lba, offset) ((((lba) >> (offset)) & 0xff))

#define ATA_READ_DMA_EXT        0x25
#define ATA_READ_DMA            0xC8
#define ATA_WRITE_DMA_EXT       0x35
#define ATA_WRITE_DMA           0xCA

#define MAX_RETRY       2

struct sata_fis_head{

    uint8_t type;
    uint8_t options;
    uint8_t status_or_cmd;
    uint8_t feat_or_err;

}__HBA_PACKED__;

struct sata_reg_fis{

    struct sata_fis_head head;

    uint8_t lba0, lba8, lba16;
    uint8_t dev;
    uint8_t lba24, lba32, lba40;

    uint8_t features;

    uint16_t count;

    uint16_t reserved[3];

}__HBA_PACKED__;

struct sata_data_fis{
    
  struct  sata_fis_head head;

  uint8_t data[0];

}__HBA_PACKED__;

void sata_create_fis(
    struct sata_reg_fis *fis,
    uint8_t   command,
    uint64_t  lba,
    uint16_t  sector_count
);

int sata_read_buffer(
    struct hba_device* dev,
    uint64_t lba,
    void  *buffer,
    uint32_t size
);


int sata_write_buffer(
    struct hba_device *dev,
    uint64_t lba,
    void *buffer,
    uint32_t size
);

#endif
