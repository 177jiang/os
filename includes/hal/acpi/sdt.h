#ifndef __jyos_sdt_h_
#define __jyos_sdt_h_

#include <stdint.h>

#define ACPI_TABLE_PACKED   __attribute__((packed))

typedef struct {
    uint32_t signature;
    uint32_t length;
    // Revision
    uint8_t rev;
    uint8_t chksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_rev;
    uint32_t vendor_id;
    uint32_t vendor_rev;
} __attribute__((packed)) acpi_sdthdr_t;

typedef struct {
    acpi_sdthdr_t header;
    acpi_sdthdr_t* entry;
} __attribute__((packed)) acpi_rsdt_t;

#endif 
