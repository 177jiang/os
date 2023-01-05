#ifndef __jyos_tss_h_
#define __jyos_tss_h_

#include <stdint.h>

struct x86_tss{

    uint32_t link;

    uint32_t esp0;
    uint16_t ss0;

    char     *__padding[94];

}__attribute__((packed));

void tss_update(uint32_t ss0, uint32_t esp0);

#endif
