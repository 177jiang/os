#ifndef __jyos_ahci_h_
#define __jyos_ahci_h_


#define AHCI_HBA_CLASS     0x10601



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

#define HBA_PxIE_DMA        (1 << 2)
#define HBA_PxIE_D2HR       (1 << 0)

#define HBA_GHC_ACHI_EN     (1 << 31)
#define HBA_GHC_INTR_EN     (1 << 1)
#define HBA_GHC_RESET       (1)


#define HBA_PxSSTS_IPM(x)        (((x) >> 8) & (0xF))
#define HBA_PxSSTS_SPD(x)        (((x) >> 4) & (0xF))
#define HBA_PxSSTS_DET(x)        ((x) & (0xF))

#define HBA_CAP_NP(x)            ((x) & (0x1F))
#define HBA_CAP_NCS(x)           (((x) >> 8) & (0xF))

typedef unsigned int  hba_reg_t;

struct ahci_port{

  hba_reg_t     *base;
  unsigned int  ssts;

  char          *cmdlstv;
  char          *fisv;

};

struct ahci_hba{

    volatile hba_reg_t  *base;
    unsigned int        port_nums;
    unsigned int        cmd_slots;
    unsigned int        version;

    struct ahci_port *ports[32];
};

void ahci_init();

void ahci_print_device();


#endif
