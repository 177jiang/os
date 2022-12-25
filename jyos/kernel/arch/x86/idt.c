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
  // _set_idt_entry(0, 0x08, _asm_isr0, 0);
  _set_idt_entry(JYOS_SYS_PANIC, 0x08, _asm_isr32, 0);
}
