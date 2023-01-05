#include <arch/x86/tss.h>
#include <constant.h>

struct x86_tss _tss = {

    .link = 0,
    .esp0 = K_STACK_START,
    .ss0  = K_DATA_SEG

};


void tss_update(uint32_t ss0, uint32_t esp0){

    _tss.ss0  = ss0;
    _tss.esp0 = esp0;

}
