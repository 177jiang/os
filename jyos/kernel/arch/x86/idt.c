#include <arch/x86/idt.h>
#include <arch/x86/interrupts.h>
#include <stdint.h>

#define IDT_ENTRY 256

uint64_t _idt[IDT_ENTRY];
uint16_t _idt_limit = sizeof(_idt) - 1;

void _set_idt_entry
  (uint16_t vector, uint16_t seg_selector, void (*isr)(), uint8_t dpl){

    uint32_t offset = (uint32_t)isr;
    _idt[vector] = (offset & 0xFFFF0000) | IDT_INT_GATE(dpl);
    _idt[vector] <<= 32;
    _idt[vector] |= (seg_selector << 16) | (offset & 0x0000FFFF);

}

void _init_idt(){
  _set_idt_entry(FAULT_DIVISION_ERROR, 0x08, _asm_isr0, 0);
  _set_idt_entry(FAULT_GENERAL_PROTECTION, 0x08, _asm_isr13, 0);
  _set_idt_entry(FAULT_PAGE_FAULT, 0x08, _asm_isr14, 0);

  _set_idt_entry(APIC_ERROR_IV, 0x08, _asm_isr250, 0);
  _set_idt_entry(APIC_LINT0_IV, 0x08, _asm_isr251, 0);
  _set_idt_entry(APIC_SPIV_IV,  0x08, _asm_isr252, 0);
  _set_idt_entry(APIC_TIMER_IV, 0x08, _asm_isr253, 0);

  /* timer */
  _set_idt_entry(RTC_TIMER_IV,  0x08, _asm_isr210, 0);

  /* keyboard */
  _set_idt_entry(PC_KBD_IV, 0x08, _asm_isr201,0);

  _set_idt_entry(JYOS_SYS_PANIC, 0x08, _asm_isr32, 0);
  _set_idt_entry(JYOS_SYS_CALL, 0x08, _asm_isr33, 0);

}
