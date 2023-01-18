#ifndef __jyos_idt_h_
#define __jyos_idt_h_


#define IDT_INT_GATE(dpl)    ( (dpl & 3) << 13 | 1 << 15 | (0xE << 8) )

#define IDT_TRAP_GATE(dpl)    ( (dpl & 3) << 13 | 1 << 15 | (0xF << 8) )

void _init_idt();

#endif
