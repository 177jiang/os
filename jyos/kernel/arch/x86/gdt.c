#include <arch/x86/gdt.h>

#include <stdint.h>

#define GDT_ENTRY 5

uint64_t _gdt[GDT_ENTRY];
uint16_t _gdt_limit = sizeof(_gdt) - 1;

void _set_gdt_entry(uint16_t index, uint32_t base, uint32_t limit, uint32_t flags) {

  _gdt[index] = ( SEG_BASE_H(base) | SEG_BASE_M(base) | SEG_LIMIT_H(limit) | flags );

  _gdt[index] <<= 32 ;

  _gdt[index] |= ( SEG_BASE_L(base) | SEG_LIMIT_L(limit) );
}

void _init_gdt() {
  
  _set_gdt_entry(0, 0, 0, 0);
  _set_gdt_entry(1, 0, 0xFFFFF, SEG_R0_CODE);
  _set_gdt_entry(2, 0, 0xFFFFF, SEG_R0_DATA);
  _set_gdt_entry(3, 0, 0xFFFFF, SEG_R3_CODE);
  _set_gdt_entry(4, 0, 0xFFFFF, SEG_R3_DATA);
  
}
