
#include <mm/mmio.h>
#include <hal/ahci.h>
#include <hal/pci.h>
#include <libc/stdio.h>

#include <mm/pmm.h>
#include <spike.h>

#include <mm/kalloc.h>

#define HBA_FIS_SIZE        256
#define HBA_CLB_SIZE        1024
#define AHCI_BAR_IN_PCI     6

static char sata_ifs[][20] = { "Not detected",
                        "SATA I (1.5Gbps)",
                        "SATA II (3.0Gbps)",
                        "SATA III (6.0Gbps)" };
static struct ahci_hba   _ahci_hba;

void ahci_print_device();

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

    // kprintf_error("%x -- %x\n", size, PCI_BAR_ADDR_MM(bar6));

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
            struct ahci_port *port =
              kmalloc(sizeof(struct ahci_port));
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
            port_addr[HBA_PxIE]  |= (HBA_PxIE_DMA | HBA_PxIE_D2HR);
            port_addr[HBA_PxCMD] |= (HBA_PxCMD_FRE | HBA_PxCMD_ST);

            _ahci_hba.ports[i] = port;
        }


        cn = (cn + 1) % 4;
        fn = (fn + 1) % 16;
        port_map >>= 1;

    }

    ahci_print_device();

}


void ahci_print_device(){

  kprintf_error("Version: %x   Ports: %d\n",
      _ahci_hba.version, _ahci_hba.port_nums);
  for(size_t i=0; i<32; ++i){

      struct ahci_port *port = _ahci_hba.ports[i];
      if(!port)continue;

      kprintf_warn("Port %d: %s\n", i, &sata_ifs[HBA_PxSSTS_SPD(port->ssts)]);

  }

}



























