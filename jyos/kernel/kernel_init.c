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

#include <datastructs/jlist.h>

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

multiboot_info_t  *_init_mb_info;
x86_page_t        *__kernel_pg_dir;

void setup_mem(multiboot_memory_map_t *map, uint32_t size);
void _kernel_post_init();
void task_1_init();
extern int _kernel_main();


void _kernel_pre_init(){

  _tty_init((void *)VGA_BUFFER_PADDR);

  intr_routine_init();
  pmm_init(MEM_1MB + ((_init_mb_info->mem_upper) << 10));
  vmm_init();

  __kernel_pg_dir       = cpu_rcr3();
  __current->page_table = __kernel_pg_dir;

}

void _kernel_init() {

  printf_("[MM] Mem: %d KiB, Extended Mem: %d KiB\n", _init_mb_info->mem_lower, _init_mb_info->mem_upper);

  uint32_t map_size = _init_mb_info->mmap_length / sizeof (multiboot_memory_map_t);
  setup_mem((multiboot_memory_map_t *)_init_mb_info->mmap_addr, map_size);

  _lock_reserved_memory();

  /*alloc stack page for kernel*/
  for(uint32_t i=0; i<(K_STACK_SIZE >> PG_SIZE_BITS); ++i){
    vmm_alloc_page(KERNEL_PID, (void*)(K_STACK_START + (i << PG_SIZE_BITS)), 0, PG_PREM_RW, 0);
  }
  printf_("[MM] Allocated %d pages for stack start at %p\n", K_STACK_SIZE>>PG_SIZE_BITS, K_STACK_START);


}

void debug(){
  struct a{
    struct list_header head;
    int v;
  }test = {.head = 0,.v = 1};

  list_init_head(&test.head);
  struct a *pos, *n;
  printf_error("%d   %x\n", test.v, test.head);
}

void _kernel_post_init(){


  /*set malloc heap*/
  assert_msg(kalloc_init() , "heap alloc failed !!\n");

  // release 4 pages for APIC and IOAPIC at 0xC0000000
  vmm_unmap_page(KERNEL_PID, (void*)(KERNEL_VSTART));
  vmm_unmap_page(KERNEL_PID, (void*)(KERNEL_VSTART + PG_SIZE));
  vmm_unmap_page(KERNEL_PID, (void*)(KERNEL_VSTART + PG_SIZE + PG_SIZE));
  vmm_unmap_page(KERNEL_PID, (void*)(KERNEL_VSTART + PG_SIZE + PG_SIZE+ PG_SIZE));

  acpi_init(_init_mb_info);

  uintptr_t ioapic_addr = acpi_get_context()->madt.ioapic->ioapic_addr;
  pmm_mark_page_occupied(KERNEL_PID, FLOOR(__APIC_BASE_PADDR, PG_SIZE_BITS), 0);
  pmm_mark_page_occupied(KERNEL_PID, FLOOR(ioapic_addr, PG_SIZE_BITS), 0);
  /* APIC_BASE_VADDR = 0xC0000000 , IOAPIC_BASE_VADDR  = oxC0000000 + 2PAGE*/
  vmm_set_mapping(KERNEL_PID, APIC_BASE_VADDR, __APIC_BASE_PADDR, PG_PREM_RW),
  vmm_set_mapping(KERNEL_PID, IOAPIC_BASE_VADDR, ioapic_addr, PG_PREM_RW),

  apic_init();
  ioapic_init();

  rtc_init();
  timer_primary_init();
  clock_init();
  ps2_kbd_init();

  sched_init();
  syscall_init();


  uint8_t * _init_end      = ROUNDUP((uint32_t)sym_vaddr(__init_hhk_end), PG_SIZE);
  uint32_t hhk_page_counts =
    ((uint32_t)(_init_end - sym_vaddr(__init_hhk_start))) >> PG_SIZE_BITS;
    /*release hhk_init_code*/
  for(uint32_t i=0; i<hhk_page_counts; ++i){
    vmm_unmap_page(KERNEL_PID, (void *)(MEM_1MB + (i << PG_SIZE_BITS)) );
  }

  printf_("hhk_init_code alloc %d pages now released\n", hhk_page_counts);

  task_1_init();


  timer_init(SYS_TIMER_FREQUENCY_HZ);

  spin();

}

void task_1_init(){

  pid_t   pid                  = alloc_pid();
  struct task_struct *new_task = get_task(pid);
  new_task->pid                = pid;
  new_task->state              = TASK_CREATED;
  new_task->created            = clock_systime();
  new_task->parent             = 0;

  list_init_head(&new_task->mm.regions.head);

  new_task->regs = (isr_param){
    .esp    = K_STACK_TOP - 20,
    .cs     = K_CODE_SEG,
    .eip    = (void*)_kernel_main,
    .ss     = K_DATA_SEG,
    .eflags = cpu_reflags()
  };
  setup_task_mem(new_task, PD_REFERENCED);

  vmm_unmount_pg_dir(PD_MOUNT_2);

  asm volatile(
      "movl %%cr3, %%eax\n"
      "movl %%esp, %%ebx\n"
      "movl %0,    %%cr3\n"
      "movl %1,    %%esp\n"
      "pushf            \n"
      "pushl %2         \n"
      "pushl %3         \n"
      "pushl $0         \n"
      "pushl $0         \n"
      "movl %%ebx, %%esp\n"
      "movl %%eax, %%cr3\n"
      ::"r"(new_task->page_table), "i"(K_STACK_TOP),
        "r"(new_task->regs.cs), "r"(new_task->regs.eip)
      :"%eax", "%ebx", "memory"
  );
  commit_task(new_task);
  // push_task(&init_task);
}


void _lock_reserved_memory(){

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
      vmm_set_mapping(KERNEL_PID, addr, addr, PG_PREM_R);
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
        PG_PREM_URW
      );

  if(vga_map_res == 0)while(1);

  _tty_init((void *)VGA_BUFFER_VADDR);

}



