#include <spike.h>
#include <arch/x86/interrupts.h>
#include <libc/stdio.h>

static char buf[1024];

void __assert_fail(const char *msg, const char *file, unsigned int line) {

    sprintf_(buf, "[ASSERT] %s (%s:%u)", msg, file, line);

    asm( "int %0" :: "i"(JYOS_SYS_PANIC), "D"(buf));

}

void panick(const char *msg){

    asm("int %0" ::"i"(JYOS_SYS_PANIC), "D"(msg));

    spin();

}
