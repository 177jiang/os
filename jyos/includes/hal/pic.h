#ifndef __jyos_pic_h_
#define __jyos_pic_h_

static inline void pic_disable() {
    // ref: https://wiki.osdev.org/8259_PIC
    asm volatile ("movb $0xff, %al\n"
        "outb %al, $0xa1\n"
        "outb %al, $0x21\n");
}

#endif
