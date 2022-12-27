
#define __ASM__ 1
#include <arch/x86/boot/multiboot.h>

#define KPG_SIZE       24*1024

.section .multiboot
  .long MB_MAGIC
  .long MB_ALIGNED_4K_MEM_MAP
  .long CHECKSUM(MB_ALIGNED_4K_MEM_MAP)

.section .bss 

  .global mb_info
  .align 16
    mb_info:
      .skip 4096

    stack_bottom:
      .skip 16318, 0
    stack_top:


.section .kpg
    .global _k_ptd
    _k_ptd:
        .skip KPG_SIZE, 0



.section .hhk_init

  .global start_

   start_:
  
  cld
  cli

    movl $stack_top, %esp
 
    
  setup_temp_gdt_:
    lgdt gdt_desc

    ljmp $8, $_load_32bit_selector

  _load_32bit_selector:
/* save multiboot */
    subl $0x10, %esp
    movl $mb_info, 4(%esp)
    movl %ebx, (%esp)
    call _save_multiboot_info

 /* init_page_table */   
    movl $(KPG_SIZE), 4(%esp)
    movl $(_k_ptd - 0xC0000000), (%esp)

    call _hhk_init

    movl (%esp), %eax
    andl $0xFFFFF000, %eax
    movl %eax, %cr3
    
    movl %cr0, %eax
    orl $0x80000000, %eax
    movl %eax, %cr0


    pushl $hhk_entry_
    ret


gdt_desc:
      .word 256*8 - 1
      .long _gdt
  _gdt:
      .quad 0x0000000000000000
      .quad 0x00C09A0000000FFF
      .quad 0x00C0920000000FFF
      .quad 0x0000000000000000
      .fill 252,8,0




