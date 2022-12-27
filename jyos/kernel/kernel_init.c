#include <arch/x86/boot/multiboot.h>
#include <mm/page.h>
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <mm/kalloc.h>

#include <hal/cpu.h>
#include <hal/rtc.h>
#include <hal/apic.h>
#include <hal/ioapic.h>
#include <hal/acpi/acpi.h>

#include <libc/stdio.h>
#include <libc/string.h>

#include <spike.h>
#include <constant.h>
#include <tty/tty.h>
#include <timer.h>


#include <stdint.h>
#include <stddef.h>


extern uint8_t __kernel_start;
extern uint8_t __kernel_end;
extern uint8_t __init_hhk_start;
extern uint8_t __init_hhk_end;
extern uint32_t _init_kernel_esp;

void setup_mem(multiboot_memory_map_t *map, uint32_t size);
void setup_stack();
void setup_kernel_runtime();


void _kernel_pre_init(multiboot_info_t *mb_info){

  _tty_init((void *)VGA_BUFFER_PADDR);

  intr_routine_init();

  uint32_t mem_size =  MEM_1MB + ((mb_info->mem_upper) << 10) ;
  pmm_init(mem_size);
  printf_("[MM] Mem: %d KiB, Extended Mem: %d KiB",
         mb_info->mem_lower, mb_info->mem_upper);
  printf_("  system has %u pages  \n",  PG_ALIGN(mem_size) >> PG_SIZE_BITS);

  vmm_init();

  uint32_t map_size = mb_info->mmap_length / sizeof (multiboot_memory_map_t);

  setup_mem((multiboot_memory_map_t *)mb_info->mmap_addr, map_size);

  setup_kernel_runtime();

}
void setup_kernel_runtime(){
  setup_stack();
  /*set malloc heap*/
  int alloc_heap = kalloc_init();
  assert_msg(alloc_heap, "heap alloc failed !!\n");

}

void _lock_reserved_memory(multiboot_info_t *mb_info){

  pid_t pid  = 0;
  multiboot_memory_map_t *mmaps = mb_info->mmap_addr;
  uint32_t map_size             = mb_info->mmap_length / sizeof(multiboot_memory_map_t);
  for(uint32_t i=0; i<map_size; ++i){
    multiboot_memory_map_t mmap = mmaps[i];
    if(mmap.type == MULTIBOOT_MEMORY_AVAILABLE){
      continue;
    }

    uint8_t *pa   = PG_ALIGN(mmap.addr_low);
    uint32_t pg_n = CEIL(mmap.len_low, PG_SIZE_BITS);
    for(uint32_t j=0; j<pg_n; ++j){
      uint32_t addr = pa + (j << PG_SIZE_BITS) ;
      vmm_set_mapping(pid, addr, addr, PG_PREM_R);
    }

  }

}

void _kernel_post_init(multiboot_info_t *mb_info){

  rtc_init();
  pid_t pid = 0;

  _lock_reserved_memory(mb_info);

  // release 4 pages for APIC and IOAPIC at 0xC0000000
  vmm_unmap_page(pid, (void*)(KERNEL_VSTART));
  vmm_unmap_page(pid, (void*)(KERNEL_VSTART + PG_SIZE));
  vmm_unmap_page(pid, (void*)(KERNEL_VSTART + PG_SIZE + PG_SIZE));
  vmm_unmap_page(pid, (void*)(KERNEL_VSTART + PG_SIZE + PG_SIZE+ PG_SIZE));

  acpi_init(mb_info);

  uintptr_t ioapic_addr = acpi_get_context()->madt.ioapic->ioapic_addr;

  pmm_mark_page_occupied(pid, FLOOR(__APIC_BASE_PADDR, PG_SIZE_BITS), 0);
  pmm_mark_page_occupied(pid, FLOOR(ioapic_addr, PG_SIZE_BITS), 0);

  /* APIC_BASE_VADDR = 0xC0000000 , IOAPIC_BASE_VADDR  = oxC0000000 + 2 */
  assert_msg(
      vmm_set_mapping(pid, APIC_BASE_VADDR, __APIC_BASE_PADDR, PG_PREM_RW),
      "Map APIC Failed\n"
      );
  assert_msg(
      vmm_set_mapping(pid, IOAPIC_BASE_VADDR, ioapic_addr, PG_PREM_RW),
      "Map IOAPIC Failed\n"
      );

  apic_init();
  ioapic_init();

  /*timer init */
  timer_init(SYS_TIMER_FREQUENCY_HZ);
  clock_init();

  /*keyboard init*/
  ps2_kbd_init();


  uint32_t hhk_page_counts =
    ((uint32_t)(sym_vaddr(__init_hhk_end) - sym_vaddr(__init_hhk_start))) >> PG_SIZE_BITS;
  for(uint32_t i=0; i<hhk_page_counts; ++i){
    vmm_unmap_page(pid, (void *)(MEM_1MB + (i << PG_SIZE_BITS)) );
  }
  printf_("hhk_init_code alloc %d pages now released\n", hhk_page_counts);
 
}

void setup_mem(multiboot_memory_map_t *map, uint32_t size){

  for(uint32_t i=0; i<size; ++i){

    multiboot_memory_map_t mmap = map[i];
    if(mmap.type == MULTIBOOT_MEMORY_AVAILABLE){
      // printf_("[MM] Base: 0x%x, len: %uK, type: %d\n",
      //     mmap.addr_low,
      //     mmap.len_low >> 10,
      //     mmap.type);
      uint32_t pgs = (mmap.addr_low + 0x0FFFU) >> PG_SIZE_BITS;
      uint32_t n   =  mmap.len_low >> PG_SIZE_BITS;
      pmm_mark_pages_free(pgs, n);
      printf_("[MM] Freed %u pages start from 0x%x\n",
             n, (mmap.addr_low + 0x0FFFU) & ~0x0fffU);
    }

  }

  /* mask kernel used pages */
  uint32_t pg_start = (V2P(sym_vaddr(__kernel_start)) >> PG_SIZE_BITS);
  size_t pg_count =
    (uint32_t)(sym_vaddr(__kernel_end) - sym_vaddr(__kernel_start)) >> PG_SIZE_BITS;
  pmm_mark_pages_occupied(0, pg_start, pg_count, 0);
  printf_("[MM] __kernel_start 0x%x.\n", sym_vaddr(__kernel_start));
  printf_("[MM] Allocated %d pages for kernel.\n", pg_count);

  /* mask vga used and vmap vga */
  pg_start = (VGA_BUFFER_PADDR >> PG_SIZE_BITS);
  pg_count = VGA_BUFFER_SIZE >> PG_SIZE_BITS;
  pmm_mark_pages_occupied(0, pg_start, pg_count, 0);
  // printf_("[MM] Allocated %d pages for vga.\n", pg_count);

  // for(uint32_t i=0; i<pg_count; ++i){
  //   vmm_map_page(
  //       0,
  //       (void*)(VGA_BUFFER_VADDR + (i << PG_SIZE_BITS)), 
  //       (void*)(VGA_BUFFER_PADDR + (i << PG_SIZE_BITS)), 
  //       PG_PREM_RW
  //   );
  // }
 
  int vga_map_res = vmm_map_pages(
        0,
        (void*)VGA_BUFFER_VADDR ,
        (void*)VGA_BUFFER_PADDR ,
        pg_count,
        PG_PREM_RW
      );

  if(vga_map_res == 0)while(1);

  _tty_init((void *)VGA_BUFFER_VADDR);

  // printf_("[MM] Mapped VGA to 0x%x \n", VGA_BUFFER_VADDR);
  // printf_("------------------------- SECOND VGA_INIT_END ----------------\n");

}

void setup_stack(){

  // for(size_t i=0; i<(K_STACK_SIZE) >> PG_SIZE_BITS; ++i){
  //
  //   vmm_alloc_page();
  //
  // }
  printf_("[MM] Allocated %d pages for stack start at 0x%x\n", ((4 * 1024 * 2) >> PG_SIZE_BITS), _init_kernel_esp);
}


void _kernel_init(multiboot_info_t *mb_info) {

  _kernel_pre_init(mb_info);
  _kernel_post_init(mb_info);
   char buf[64];
   cpu_get_brand(buf);
   printf_("CPU: %s\n\n", buf);
   printf_("------------------------- Initialization end !!! ----------------\n");

}
