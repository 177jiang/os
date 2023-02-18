
#include <hal/ahci/ahci.h>
#include <hal/ahci/hba.h>
#include <hal/ahci/scsi.h>
#include <hal/ahci/sata.h>
#include <hal/pci.h>
#include <block.h>


#include <mm/mmio.h>
#include <mm/pmm.h>
#include <mm/kalloc.h>

#include <stdint.h>
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

__hba_reset_port(hba_reg_t *port_base){

    port_base[HBA_PxCMD] &= ~HBA_PxCMD_ST;
    port_base[HBA_PxCMD] &= ~HBA_PxCMD_FRE;
    int cnt = wait_until_expire(!(port_base[HBA_PxCMD] & HBA_PxCMD_CR), 500000);

    if (cnt) return;
    // 如果port未响应，则继续执行重置
    port_base[HBA_PxSCTL] = (port_base[HBA_PxSCTL] & ~0xf) | 1;
    io_delay(100000); //等待至少一毫秒，差不多就行了
    port_base[HBA_PxSCTL] &= ~0xf;
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
    // _ahci_hba.base[HBA_GHC] |= HBA_GHC_RESET;
    // wait_until(!(_ahci_hba.base[HBA_GHC] & HBA_GHC_RESET));

    //enable
    _ahci_hba.base[HBA_GHC] |= (HBA_GHC_ACHI_EN | HBA_GHC_INTR_EN);

    hba_reg_t cap       =  _ahci_hba.base[HBA_CAP];
    hba_reg_t port_map  =  _ahci_hba.base[HBA_PI];
    _ahci_hba.port_nums =  HBA_CAP_NP(cap) + 1;
    _ahci_hba.cmd_slots =  HBA_CAP_NCS(cap);
    _ahci_hba.version   =  _ahci_hba.base[HBA_VERSION];
    _ahci_hba.port_map  = port_map;

    // set per port
    uintptr_t clb_vaddr =  0, fis_vaddr =  0;
    uintptr_t clb_paddr =  0, fis_paddr =  0;

    for(size_t i=0,cn = 0, fn = 0; i<32; ++i){

        if(port_map & 1){
            struct hba_port *port =
              valloc(sizeof(struct hba_port));
            hba_reg_t *port_addr   =
              (_ahci_hba.base + HBA_PORT_BASE + i * HBA_PORT_SIZE);

            __hba_reset_port(port_addr);

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
            port_addr[HBA_PxIE]  |= (HBA_PxIE_D2HR);

            _ahci_hba.ports[i] = port;

            if(HBA_PxSSTS_SPD(port->ssts)){

                wait_until(!(port_addr[HBA_PxCMD] & HBA_PxCMD_CR));
                port_addr[HBA_PxCMD] |= (HBA_PxCMD_FRE | HBA_PxCMD_ST);

                if(!ahci_init_device(port)){
                    kprintf_error("Fail to init device\n");
                }

                block_mount_disk(port->device);
            }
        }


        cn = (cn + 1) % 4;
        fn = (fn + 1) % 16;
        port_map >>= 1;

    }
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

      kprintf_warn("     capacity: %d KiB\n",
              (dev->max_lba * dev->block_size) >> 10);
      kprintf_warn("     block size: %dB\n", dev->block_size);
      kprintf_warn("     block/sector: %d\n", dev->block_per_sec);
      kprintf_warn("     alignment: %dB\n", dev->alignment_offset);
      kprintf_warn("     capabilities: %x\n", dev->capabilities);
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

void sata_create_fis(
        struct sata_reg_fis *fis,
        uint8_t   command,
        uint64_t  lba,
        uint16_t  sector_count){

    fis->head.type          =  SATA_REG_FIS_H2D;
    fis->head.options       =  SATA_REG_FIS_COMMAND;
    fis->head.status_or_cmd =  command;

    fis->dev   =  0;

    fis->lba0  = SATA_LBA_COMPONENT(lba, 0);
    fis->lba8  = SATA_LBA_COMPONENT(lba, 8);
    fis->lba16 = SATA_LBA_COMPONENT(lba, 16);
    fis->lba24 = SATA_LBA_COMPONENT(lba, 24);

    fis->lba32 = SATA_LBA_COMPONENT(lba, 32);
    fis->lba40 = SATA_LBA_COMPONENT(lba, 40);

    fis->count = sector_count;
}

int hba_prepare_cmd(
        struct hba_port *port,
        struct hba_cmd_table **cmdt,
        struct hba_cmd_header **cmdh,
        void *buffer,
        uint32_t size){

    assert_msg(size <= 0x400000, "HBA: buffer too big\n");
    int slot = __get_free_slot(port);
    assert_msg(slot >= 0, "HBA : no cmd slot\n");


    struct hba_cmd_header *ch = port->cmdlstv + slot;
    struct hba_cmd_table  *ct = valloc_dma(sizeof(struct hba_cmd_table));
    memset(ch, 0, sizeof(*ch));
    memset(ct, 0, sizeof(*ct));

    ch->cmd_table_base =  vmm_v2p(ct);
    ch->options        = 
      HBA_CMDH_FIS_LEN(sizeof(struct sata_reg_fis)) | HBA_CMDH_CLR_BUSY;
    if(buffer){
        ch->prdt_len   =  1;
        ct->entries[0] = (struct hba_prdte){
            .data_base  = vmm_v2p(buffer),
            .byte_count = size - 1
        };
    }

    *cmdt = ct, *cmdh = ch;

    return slot;
}

int ahci_init_device(struct hba_port *port){


    struct hba_cmd_header *cmdh;
    struct hba_cmd_table  *cmdt;

    wait_until(!(port->base[HBA_PxTFD] & (HBA_TFD_BSY)));

    uint16_t *data = valloc_dma(512);

    int slot = hba_prepare_cmd(port, &cmdt, &cmdh, data, 512);

    port->base[HBA_PxIS] = 0;
    port->device         =  valloc(sizeof(struct hba_device));
    memset(port->device, 0, sizeof(struct hba_device));

    port->device->port   =  port;

    //fis
    struct sata_reg_fis *cfis = cmdt->CFIS;

    if(port->base[HBA_PxSIG] == HBA_DEV_SIG_ATA){

        sata_create_fis(cfis, ATA_IDENTIFY_DEVICE, 0, 0);
    }else{
        port->device->signature |= HBA_DEV_FATAPI;
        sata_create_fis(cfis, ATA_IDENTIFY_PAKCET_DEVICE, 0, 0);
    }

    port->base[HBA_PxCI] = (1 << slot);
    wait_until(!(port->base[HBA_PxCI] & (1 << slot)));

    if((port->base[HBA_PxTFD] & HBA_TFD_ERR)){
        goto fail;
    }

    ahci_parse_dev_info(port->device, data);

    if(port->device->signature & HBA_DEV_FATAPI){

        sata_create_fis(cfis, ATA_PACKET, 512 << 8, 0);
        struct scsi_cdb16 *cdb16 = cmdt->ACMD;
        scsi_create_packet16(cdb16, SCSI_READ_BLOCKS_16, 0, 512);
        cdb16->misc1 = 0x10;
        cmdh->transferred_byte_size = 0;
        cmdh->options |= HBA_CMDH_ATAPI;

        port->base[HBA_PxCI] = (1 << slot);
        wait_until(!(port->base[HBA_PxCI] & (1 << slot)));

        if((port->base[HBA_PxTFD] & HBA_TFD_ERR)){
            goto fail;
        }

        scsi_parse_capacity(port->device, data);
    }


    ahci_register_ops(port);
    vfree_dma(data);
    vfree_dma(cmdt);
    return 1;

fail:
    vfree_dma(data);
    vfree_dma(cmdt);
    return 0;
}
int ahci_identify_device(struct hba_port *port){
    vfree(port->device);
    return ahci_init_device(port);
}

void ahci_register_ops(struct hba_port *port){

    port->device->ops.identify = ahci_identify_device;

    if(!(port->device->signature & HBA_DEV_FATAPI)){

        port->device->ops.read_buffer  = sata_read_buffer;
        port->device->ops.write_buffer = sata_write_buffer;

    }else{

        port->device->ops.read_buffer  = scsi_read_buffer;
        port->device->ops.write_buffer = scsi_write_buffer;
    }

}

struct hba_port * ahci_get_port(unsigned int i){

    if(i >= 32)return NULL;

    return _ahci_hba.ports[i];

}




























