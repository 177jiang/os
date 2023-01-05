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
#include <sched.h>
#include <syscall.h>


#include <stdint.h>
#include <stddef.h>


extern uint8_t __kernel_start;
extern uint8_t __kernel_end;
extern uint8_t __init_hhk_start;
extern uint8_t __init_hhk_end;


void setup_mem(multiboot_memory_map_t *map, uint32_t size);
void _kernel_post_init();
extern int _kernel_main();

multiboot_info_t* _init_mb_info;

void _kernel_pre_init(){

  uint32_t mem_size =  MEM_1MB + ((_init_mb_info->mem_upper) << 10) ;
  uint32_t map_size = _init_mb_info->mmap_length / sizeof (multiboot_memory_map_t);

  _tty_init((void *)VGA_BUFFER_PADDR);
  intr_routine_init();
  pmm_init(mem_size);
  vmm_init();
  setup_mem((multiboot_memory_map_t *)_init_mb_info->mmap_addr, map_size);
  rtc_init();

  printf_("[MM] Mem: %d KiB, Extended Mem: %d KiB",
      _init_mb_info->mem_lower, _init_mb_info->mem_upper);

  /*alloc stack page for kernel*/
  for(uint32_t i=0; i<(K_STACK_SIZE >> PG_SIZE_BITS); ++i){
    vmm_alloc_page(0, (void*)(K_STACK_START + (i << PG_SIZE_BITS)), 0, PG_PREM_RW, 0);
  }

  sched_init();
  task_1_init();

  // while(1);
}

void task_1_init(){

  struct task_struct init_task;
  uint32_t *ik_stack = (uint32_t *)K_STACK_TOP - sizeof(uint32_t) * 5;

  memset(&init_task, 0, sizeof(init_task));

  init_task.page_table = (void*)cpu_rcr3();
  init_task.pid        = alloc_pid();
  init_task.regs       = (isr_param){
    .esp    = ik_stack,
    .cs     = K_CODE_SEG,
    .eip    = (void*)_kernel_post_init,
    .ss     = K_DATA_SEG,
    .eflags = cpu_reflags()
  };

  ik_stack[4] = init_task.regs.eflags;
  ik_stack[3] = init_task.regs.cs;
  ik_stack[2] = init_task.regs.eip;

  push_task(&init_task);

  schedule();

}


void _kernel_post_init(){

  /*set malloc heap*/
  assert_msg( kalloc_init() , "heap alloc failed !!\n");

  pid_t pid = 0;

  _lock_reserved_memory();

  // release 4 pages for APIC and IOAPIC at 0xC0000000
  vmm_unmap_page(pid, (void*)(KERNEL_VSTART));
  vmm_unmap_page(pid, (void*)(KERNEL_VSTART + PG_SIZE));
  vmm_unmap_page(pid, (void*)(KERNEL_VSTART + PG_SIZE + PG_SIZE));
  vmm_unmap_page(pid, (void*)(KERNEL_VSTART + PG_SIZE + PG_SIZE+ PG_SIZE));

  printf_("kamlloc_test  kheap %x \n", __current->mm.kernel_heap);

  acpi_init(_init_mb_info);

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

  syscall_install();


  uint32_t hhk_page_counts =
    ((uint32_t)(sym_vaddr(__init_hhk_end) - sym_vaddr(__init_hhk_start))) >> PG_SIZE_BITS;

    /**/
  for(uint32_t i=0; i<hhk_page_counts; ++i){
    vmm_unmap_page(pid, (void *)(MEM_1MB + (i << PG_SIZE_BITS)) );
  }

  printf_("%x\n", sym_vaddr(__init_hhk_end));
  printf_("hhk_init_code alloc %d pages now released\n", hhk_page_counts);

  _kernel_main();

}



void _lock_reserved_memory(){

  pid_t pid  = 0;
  multiboot_memory_map_t *mmaps = _init_mb_info->mmap_addr;
  uint32_t map_size             = _init_mb_info->mmap_length / sizeof(multiboot_memory_map_t);
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

void setup_mem(multiboot_memory_map_t *map, uint32_t size){

  for(uint32_t i=0; i<size; ++i){

    multiboot_memory_map_t mmap = map[i];
    if(mmap.type == MULTIBOOT_MEMORY_AVAILABLE){
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
 
  int vga_map_res = vmm_map_pages(
        0,
        (void*)VGA_BUFFER_VADDR ,
        (void*)VGA_BUFFER_PADDR ,
        pg_count,
        PG_PREM_RW
      );

  if(vga_map_res == 0)while(1);

  _tty_init((void *)VGA_BUFFER_VADDR);

}



void _kernel_init() {
  _kernel_pre_init();
  _kernel_post_init();
   char buf[64];
   cpu_get_brand(buf);
   printf_("CPU: %s\n\n", buf);
   printf_("------------------------- Initialization end !!! ----------------\n");
}
