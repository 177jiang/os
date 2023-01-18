#ifndef __jyos_madt_parser_h_
#define __jyos_madt_parser_h_

#include <hal/acpi/acpi.h>

/**
 * @brief Parse the MADT and populated into main TOC
 * 
 * @param rsdt RSDT
 * @param toc The main TOC
 */
void madt_parse(acpi_madt_t* madt, acpi_context* toc);

#endif /* __LUNAIX_PARSER_MADT_PARSER_H */
