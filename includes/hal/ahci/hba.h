#ifndef __jyos_hba_h_
#define __jyos_hba_h_

#include <stdint.h>

#define HBA_CAP         (0x00 >>  2)
#define HBA_GHC         (0x04 >>  2)
#define HBA_IS          (0x08 >>  2)
#define HBA_PI          (0x0c >>  2)
#define HBA_VERSION     (0x10 >>  2)

#define HBA_PORT_BASE   (0x100 >> 2)
#define HBA_PORT_SIZE   (0x080 >> 2)
                             
                            
#define HBA_PxCLB       (0x00  >> 2)
                          
#define HBA_PxFB        (0x08  >> 2)
                           
#define HBA_PxIS        (0x10  >> 2)
#define HBA_PxIE        (0x14  >> 2)
#define HBA_PxCMD       (0x18  >> 2)
                         
#define HBA_PxTFD       (0x20  >> 2)
#define HBA_PxSIG       (0x24  >> 2)
#define HBA_PxSSTS      (0x28  >> 2)
#define HBA_PxSCTL      (0x2c  >> 2)
#define HBA_PxSERR      (0x30  >> 2)
#define HBA_PxSACT      (0x34  >> 2)
#define HBA_PxCI        (0x38  >> 2)
#define HBA_PxSNTF      (0x3c  >> 2)
#define HBA_PxFBS       (0x40  >> 2)
#define HBA_PxDEVSLP    (0x44  >> 2)
                        
#define HBA_PxVS        (0x70  >> 2)

#define HBA_PxCMD_FRE       (1 << 4)
#define HBA_PxCMD_ST        (1 << 0)
#define HBA_PxCMD_CR        (1 << 15)
#define HBA_PxCMD_FR        (1 << 14)

#define HBA_PxIE_DMA        (1 << 2)
#define HBA_PxIE_D2HR       (1 << 0)

#define HBA_GHC_ACHI_EN     (1 << 31)
#define HBA_GHC_INTR_EN     (1 << 1)
#define HBA_GHC_RESET       (1)


#define HBA_PxSSTS_IPM(x)        (((x) >> 8) & (0xF))
#define HBA_PxSSTS_SPD(x)        (((x) >> 4) & (0xF))
#define HBA_PxSSTS_DET(x)        ((x) & (0xF))

#define HBA_CAP_NP(x)            ((x) & (0x1F))
#define HBA_CAP_NCS(x)           (((x) >> 8) & (0x1F))

#define HBA_CMDH_FIS_LEN(fis_bytes)         (((fis_bytes) / 4) & 0x1f)
#define HBA_CMDH_ATAPI                      (1 << 5)
#define HBA_CMDH_WRITE                      (1 << 6)
#define HBA_CMDH_PREFETCH                   (1 << 7)
#define HBA_CMDH_R                          (1 << 8)
#define HBA_CMDH_CLR_BUSY                   (1 << 10)
#define HBA_CMDH_PRDT_LEN(entries)          (((entries)&0xffff) << 16)

#define HBA_DEV_SIG_ATAPI       0xEB140101
#define HBA_DEV_SIG_ATA         0x00000101

#define __HBA_PACKED__ __attribute__((packed))


typedef unsigned int  hba_reg_t;

struct hba_cmd_header{

    uint16_t options;
    uint16_t prdt_len;
    uint32_t transferred_byte_size;
    uint32_t cmd_table_base;
    uint32_t reserved[5];

}__HBA_PACKED__;

struct hba_prdte{

  uint32_t data_base;
  uint32_t data_base_upper_32;
  uint32_t reserved;
  uint32_t byte_count;

}__HBA_PACKED__;

struct hba_cmd_table{

    uint8_t CFIS[0x40];
    uint8_t ACMD[0x10];
    uint8_t reserved[0x30];
    struct hba_prdte entries[3];
}__HBA_PACKED__;

struct hba_device{

    char          serial_num[20];
    char          model[40];
    uint32_t      signature;
    uint32_t      max_lba;
    uint32_t      block_size;
    uint8_t       wwn[8];
    uint8_t       cbd_size;

};

struct hba_port{

  hba_reg_t     *base;
  unsigned int  ssts;

  char          *cmdlstv;
  char          *fisv;

  struct hba_device *device;

};

struct ahci_hba{

    volatile hba_reg_t  *base;
    unsigned int        port_nums;
    unsigned int        cmd_slots;
    unsigned int        version;

    struct hba_port *ports[32];
};
#endif

