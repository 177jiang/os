#include <arch/x86/idt.h>
#include <arch/x86/interrupts.h>
#include <stdint.h>

#define IDT_ENTRY 256

uint64_t _idt[IDT_ENTRY];
uint16_t _idt_limit = sizeof(_idt) - 1;

#define _set_idt_entry(vector,seg_selector,isr, dpl, T)                         \
    _idt[vector] = ((uint32_t)isr & 0xFFFF0000) | IDT_##T##_GATE(dpl);          \
    _idt[vector] <<= 32;                                                        \
    _idt[vector] |= (seg_selector << 16) | ((uint32_t)isr& 0x0000FFFF);



void _init_idt(){

  _set_idt_entry(FAULT_DIVISION_ERROR, 0x08, _asm_isr0,      0, INT);
  _set_idt_entry(FAULT_GENERAL_PROTECTION, 0x08, _asm_isr13, 0, INT);
  _set_idt_entry(FAULT_PAGE_FAULT, 0x08, _asm_isr14,         0, INT);
  _set_idt_entry(FAULT_STACK_SEG_FAULT, 0x08, _asm_isr12,    0, INT);

  _set_idt_entry(APIC_ERROR_IV, 0x08, _asm_isr250, 0, INT);
  _set_idt_entry(APIC_LINT0_IV, 0x08, _asm_isr251, 0, INT);
  _set_idt_entry(APIC_SPIV_IV,  0x08, _asm_isr252, 0, INT);
  _set_idt_entry(APIC_TIMER_IV, 0x08, _asm_isr253, 0, INT);

  /* timer */
  _set_idt_entry(RTC_TIMER_IV,  0x08, _asm_isr210, 0, INT);

  /* keyboard */
  _set_idt_entry(PC_KBD_IV, 0x08, _asm_isr201, 0, INT);

  _set_idt_entry(JYOS_SYS_PANIC, 0x08, _asm_isr32, 0, INT);

  /*system call */
  _set_idt_entry(JYOS_SYS_CALL, 0x08, _asm_isr33, 3, INT);

}
