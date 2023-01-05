#include <syscall.h>

extern void syscall_handler(isr_param *param);

void syscall_install() {
    intr_setvector(JYOS_SYS_CALL, syscall_handler);
}
