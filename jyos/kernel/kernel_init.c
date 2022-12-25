#include <arch/x86/boot/multiboot.h>
#include <jyos/constant.h>
#include <jyos/tty/tty.h>
#include <jyos/mm/page.h>
#include <jyos/mm/vmm.h>
#include <jyos/mm/pmm.h>
#include <jyos/mm/kalloc.h>
#include <hal/cpu.h>
#include <libc/stdio.h>
#include <jyos/spike.h>

#include <stdint.h>
#include <stddef.h>


extern uint8_t __kernel_start;
extern uint8_t __kernel_end;
extern uint8_t __init_hhk_end;
extern uint32_t _init_kernel_esp;

void setup_mem(multiboot_memory_map_t *map, uint32_t size);
void setup_stack();

void _kernel_pre_init(multiboot_info_t *mb_info){

  _tty_init((void *)VGA_BUFFER_PADDR);

  printf("-------- FIRST VGA_INIT_END ----------------\n");


  uint32_t mem_size =  MEM_1MB + ((mb_info->mem_upper) << 10) ;
  pmm_init(mem_size);
  printf("[MM] Mem: %d KiB, Extended Mem: %d KiB",
         mb_info->mem_lower, mb_info->mem_upper);
  printf("  system has %u pages  \n",  PG_ALIGN(mem_size) >> PG_SIZE_BITS);
  printf("------- PMM_INIT_END ----------------");

  vmm_init();
  printf("------ VMM_INIT_END ----------------\n");

  uint32_t map_size = mb_info->mmap_length / sizeof (multiboot_memory_map_t);

  setup_mem((multiboot_memory_map_t *)mb_info->mmap_addr, map_size);

  setup_stack();

}

void _kernel_post_init(){

  /*unmap 0 - 1mb and 1mb - hhk_init*/
  uint32_t hhk_init_pg_count  = ((uint32_t)(sym_vaddr(__init_hhk_end))) >> PG_SIZE_BITS;
  printf("[MM] Releaseing %d pages from 0x0.\n", hhk_init_pg_count);

  for(uint32_t i=0; i<hhk_init_pg_count; ++i){
    vmm_unmap_page(0, (void *)(i << PG_SIZE_BITS));
  }

  /*set heap */
  int alloc_heap = kmalloc_init();
  assert_msg(alloc_heap, "heap alloc failed !!\n");
}

void setup_mem(multiboot_memory_map_t *map, uint32_t size){

  for(uint32_t i=0; i<size; ++i){
    multiboot_memory_map_t mmap = map[i];
    printf("[MM] Base: 0x%x, len: %uK, type: %d\n",
        mmap.addr_low,
        mmap.len_low >> 10,
        mmap.type);

    if(mmap.type == MULTIBOOT_MEMORY_AVAILABLE){
      uint32_t pgs = (mmap.addr_low + 0x0FFFU) >> PG_SIZE_BITS;
      uint32_t n   =  mmap.len_low >> PG_SIZE_BITS;
      pmm_mark_pages_free(pgs, n);
      printf("[MM] Freed %u pages start from 0x%x\n",
             n, (mmap.addr_low + 0x0FFFU) & ~0x0fffU);
    }

  }

  /* mask kernel used pages */
  uint32_t pg_start = (V2P(sym_vaddr(__kernel_start)) >> PG_SIZE_BITS);
  size_t pg_count =
    (uint32_t)(sym_vaddr(__kernel_end) - sym_vaddr(__kernel_start)) >> PG_SIZE_BITS;
  pmm_mark_pages_occupied(0, pg_start, pg_count, 0);
  printf("[MM] __kernel_start 0x%x.\n", sym_vaddr(__kernel_start));
  printf("[MM] Allocated %d pages for kernel.\n", pg_count);

  /* mask vga used and vmap vga */
  pg_start = (VGA_BUFFER_PADDR >> PG_SIZE_BITS);
  pg_count = VGA_BUFFER_SIZE >> PG_SIZE_BITS;
  pmm_mark_pages_occupied(0, pg_start, pg_count, 0);
  printf("[MM] Allocated %d pages for vga.\n", pg_count);

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

  printf("[MM] Mapped VGA to 0x%x \n", VGA_BUFFER_VADDR);
  printf("------------------------- SECOND VGA_INIT_END ----------------\n");

}

void setup_stack(){

  // for(size_t i=0; i<(K_STACK_SIZE) >> PG_SIZE_BITS; ++i){
  //
  //   vmm_alloc_page();
  //
  // }
  printf("[MM] Allocated %d pages for stack start at 0x%x\n", (4 * 1024 * 2), _init_kernel_esp);
  printf("------------------ STACK_INIT_END-----------------------\n");
}

void _kernel_init(multiboot_info_t *mb_info) {

  _kernel_pre_init(mb_info);
  _kernel_post_init();
   printf("------------------------- Initialization end !!! ----------------\n");
   char buf[64];
   cpu_get_brand(buf);
   printf("CPU: %s\n\n", buf);

}
