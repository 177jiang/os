#ifndef __jyos_ahci_h_
#define __jyos_ahci_h_


#define AHCI_HBA_CLASS     0x10601

#define ATA_IDENTIFY_DEVICE         0xEC
#define ATA_IDENTIFY_PAKCET_DEVICE  0xA1
#define ATA_PACKET                  0xA0




void ahci_init();

void ahci_print_device();


#endif
