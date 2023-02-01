
#include <hal/ahci/ahci.h>
#include <hal/ahci/hba.h>
#include <hal/ahci/scsi.h>
#include <hal/ahci/sata.h>
#include <hal/pci.h>


#include <mm/mmio.h>
#include <mm/pmm.h>
#include <mm/kalloc.h>

#include <spike.h>
#include <libc/stdio.h>

#define HBA_FIS_SIZE        256
#define HBA_CLB_SIZE        1024
#define AHCI_BAR_IN_PCI     6

static char sata_ifs[][20] = { "Not detected",
                        "SATA I (1.5Gbps)",
                        "SATA II (3.0Gbps)",
                        "SATA III (6.0Gbps)" };
static struct ahci_hba   _ahci_hba;

void ahci_print_device();

void __do_ahci_hba_intr(){
    kprintf_error("-----------------------------------------AHCI INT\n");
}

void ahci_init(){

    struct pci_device *ahci = pci_get_device_by_class(AHCI_HBA_CLASS);

    assert_msg(ahci, "AHIC: NOt Fount\n");
    
    uintptr_t bar6, size;

    size = pci_bar_sizing(ahci, &bar6, AHCI_BAR_IN_PCI);

    assert_msg(size && PCI_BAR_MMIO(bar6), "AHCI: BAR_6 is not MMIO\n");

    pci_reg_t cmd = pci_read_cspace(ahci->cspace_base, PCI_REG_STATUS_CMD);

    // 禁用传统中断（因为我们使用MSI），启用MMIO访问，允许PCI设备间访问
    cmd |= (PCI_RCMD_MM_ACCESS | PCI_RCMD_DISABLE_INTR | PCI_RCMD_BUS_MASTER);
    pci_write_cspace(ahci->cspace_base, PCI_REG_STATUS_CMD, cmd);

    pci_setup_msi(ahci, AHCI_HBA_IV);
    intr_setvector(AHCI_HBA_IV, __do_ahci_hba_intr);

    memset(&_ahci_hba, 0, sizeof(_ahci_hba));
    _ahci_hba.base = ioremap(PCI_BAR_ADDR_MM(bar6), size);

    //reset
    _ahci_hba.base[HBA_GHC] |= HBA_GHC_RESET;
    wait_until(!(_ahci_hba.base[HBA_GHC] & HBA_GHC_RESET));

    //enable
    _ahci_hba.base[HBA_GHC] |= (HBA_GHC_ACHI_EN | HBA_GHC_INTR_EN);

    hba_reg_t cap       =  _ahci_hba.base[HBA_CAP];
    _ahci_hba.port_nums =  HBA_CAP_NP(cap) + 1;
    _ahci_hba.cmd_slots =  HBA_CAP_NCS(cap);
    _ahci_hba.version   =  _ahci_hba.base[HBA_VERSION];

    // set per port
    hba_reg_t port_map  =  _ahci_hba.base[HBA_PI];
    uintptr_t clb_vaddr =  0, fis_vaddr =  0;
    uintptr_t clb_paddr =  0, fis_paddr =  0;

    for(size_t i=0,cn = 0, fn = 0; i<32; ++i){

        if(port_map & 1){
            struct hba_port *port =
              valloc(sizeof(struct hba_port));
            hba_reg_t *port_addr   =
              (_ahci_hba.base + HBA_PORT_BASE + i * HBA_PORT_SIZE);

            if(!cn){
                clb_paddr = pmm_alloc_page(KERNEL_PID,  PPG_LOCKED);
                clb_vaddr = ioremap(clb_paddr, PG_SIZE);
                memset(clb_vaddr, 0, PG_SIZE);
            }

            if(!fn){
                fis_paddr = pmm_alloc_page(KERNEL_PID, PPG_LOCKED);
                fis_vaddr = ioremap(fis_paddr, PG_SIZE);
                memset(fis_vaddr, 0, PG_SIZE);
            }

            port_addr[HBA_PxCLB] =  clb_paddr + cn * HBA_CLB_SIZE;
            port_addr[HBA_PxFB]  =  fis_paddr + fn * HBA_FIS_SIZE;

            port->base    =  port_addr;
            port->ssts    =  port_addr[HBA_PxSSTS];
            port->cmdlstv =  clb_vaddr + cn * HBA_CLB_SIZE;
            port->fisv    =  fis_vaddr + fn * HBA_FIS_SIZE;

            port_addr[HBA_PxCI]   = 0;
            port_addr[HBA_PxIE]   = -1;
            port_addr[HBA_PxIE]  |= (HBA_PxIE_DMA | HBA_PxIE_D2HR);

            if(HBA_PxSSTS_SPD(port->ssts)){

                wait_until(!(port_addr[HBA_PxCMD] & HBA_PxCMD_CR));
                port_addr[HBA_PxCMD] |= (HBA_PxCMD_FRE | HBA_PxCMD_ST);

                if(!ahci_init_device(port)){
                    kprintf_error("Fail to init device\n");
                }

            }

            _ahci_hba.ports[i] = port;
        }


        cn = (cn + 1) % 4;
        fn = (fn + 1) % 16;
        port_map >>= 1;

    }

    ahci_print_device();

}


void ahci_print_device(){

  kprintf_error("Version: %x   Ports: %d, Slot: %d \n",
      _ahci_hba.version, _ahci_hba.port_nums, _ahci_hba.cmd_slots);
  for(size_t i=0; i<32; ++i){

      struct hba_port *port = _ahci_hba.ports[i];
      if(!port)continue;

      kprintf_warn("Port %d: %s (%x)\n",
              i, &sata_ifs[HBA_PxSSTS_SPD(port->ssts)], port->base[HBA_PxSIG]);

      struct hba_device *dev = port->device;

      if(!HBA_PxSSTS_SPD(port->ssts) || !dev){
          continue;
      }

      kprintf_warn("  capacity: %d KiB\n",
              (dev->max_lba * dev->block_size) >> 10);
      kprintf_warn("     sector size: %dB\n", dev->block_size);
      kprintf_warn("     model: %s\n", &dev->model);
      kprintf_warn("     serial: %s\n", &dev->serial_num);

  }

}

int __get_free_slot(struct hba_port *port){

    hba_reg_t sact = port->base[HBA_PxSACT];
    hba_reg_t ci   = port->base[HBA_PxCI];

    uint32_t map = sact | ci, i = 0;

    for( ;
        i<=_ahci_hba.cmd_slots && (map & 1);
        ++i, map >>= 1);

    return i | -(i > _ahci_hba.cmd_slots) ;
}

void
sata_create_fis(
        struct sata_reg_fis *fis,
        uint8_t   command,
        uint32_t  lba_lo,
        uint32_t  lba_hi,
        uint16_t  sector_count){

    fis->head.type          =  SATA_REG_FIS_H2D;
    fis->head.options       =  SATA_REG_FIS_COMMAND;
    fis->head.status_or_cmd =  command;

    fis->dev   =  0;

    fis->lba0  = SATA_LBA_COMPONENT(lba_lo, 0);
    fis->lba8  = SATA_LBA_COMPONENT(lba_lo, 8);
    fis->lba16 = SATA_LBA_COMPONENT(lba_lo, 16);
    fis->lba24 = SATA_LBA_COMPONENT(lba_lo, 24);

    fis->lba32 = SATA_LBA_COMPONENT(lba_hi, 0);
    fis->lba40 = SATA_LBA_COMPONENT(lba_hi, 8);

    fis->count = sector_count;
}

int hba_alloc_slot(
        struct hba_port *port,
        struct hba_cmd_table **cmdt,
        struct hba_cmd_header **cmdh,
        uint16_t options){

    int slot = __get_free_slot(port);

    assert_msg(slot >= 0, "HBA : No cmd slot");


    struct hba_cmd_header *ch = port->cmdlstv + slot;
    struct hba_cmd_table  *ct = valloc_dma(sizeof(struct hba_cmd_table));

    memset(ch, 0, sizeof(*ch));
    memset(ct, 0, sizeof(*ct));

    ch->prdt_len       =  1;
    ch->cmd_table_base =  vmm_v2p(ct);
    ch->prdt_len       =  HBA_CMDH_CLR_BUSY | (options & ~0x1F);

    *cmdt = ct, *cmdh = ch;

    return slot;
}

int ahci_init_device(struct hba_port *port){


    struct hba_cmd_header *cmdh;
    struct hba_cmd_table  *cmdt;

    int slot = hba_alloc_slot(port, &cmdt, &cmdh, 0);

    port->base[HBA_PxIS]    = 0;
    port->device            =  valloc(sizeof(struct hba_device));
    port->device->signature =  port->base[HBA_PxSIG];

    uint16_t *data = valloc_dma(512);

    cmdt->entries[0] = (struct hba_prdte){
        .data_base  = vmm_v2p(data),
        .byte_count = 511
    };
    cmdh->prdt_len = 1;

    //fis
    struct sata_reg_fis *cfis = cmdt->CFIS;

    if(port->device->signature == HBA_DEV_SIG_ATA){

        sata_create_fis(cfis, ATA_IDENTIFY_DEVICE, 0, 0, 0);
    }else{

        sata_create_fis(cfis, ATA_IDENTIFY_PAKCET_DEVICE, 0, 0, 0);
    }

    port->base[HBA_PxCI] = (1 << slot);
    wait_until(!(port->base[HBA_PxCI] & (1 << slot)));

    ahci_parse_dev_info(port->device, data);

    if(port->device->signature != HBA_DEV_SIG_ATA){

        sata_create_fis(cfis, ATA_PACKET, 512 << 8, 0, 0);
        struct scsi_cdb16 *cdb16 = cmdt->ACMD;
        scsi_create_packet16(cdb16, SCSI_READ_BLOCKS_16, 0, 0, 512);
        cdb16->misc1 = 0x10;
        cmdh->transferred_byte_size = 0;
        cmdh->options = HBA_CMDH_ATAPI;

        port->base[HBA_PxCI] = (1 << slot);
        wait_until(!(port->base[HBA_PxCI] & (1 << slot)));

        scsi_parse_capacity(port->device, data);
    }


    vfree_dma(data);
    vfree_dma(cmdt);
    return 1;

}


























