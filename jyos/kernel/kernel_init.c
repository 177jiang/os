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
extern uint8_t __user_text_start;
extern uint8_t __user_text_end;

multiboot_info_t  *_init_mb_info;
x86_page_t        *__kernel_pg_dir;

void setup_mem(multiboot_memory_map_t *map, uint32_t size);
void _kernel_post_init();
void task_1_init();

extern int _kernel_main();
extern int task_1_work();


void _kernel_pre_init(){


  intr_routine_init();
  pmm_init(MEM_1MB + ((_init_mb_info->mem_upper) << 10));
  vmm_init();

  __kernel_pg_dir       = cpu_rcr3();
  __current->page_table = __kernel_pg_dir;

  _tty_init((void *)VGA_BUFFER_PADDR);

}

void _kernel_init() {

  printf_("[MM] Mem: %d KiB, Extended Mem: %d KiB\n", _init_mb_info->mem_lower, _init_mb_info->mem_upper);

  uint32_t map_size = _init_mb_info->mmap_length / sizeof (multiboot_memory_map_t);
  setup_mem((multiboot_memory_map_t *)_init_mb_info->mmap_addr, map_size);


  /*alloc stack page for kernel*/
  for(uint32_t i=0; i<(K_STACK_SIZE >> PG_SIZE_BITS); ++i){
    uint32_t pa = pmm_alloc_page(KERNEL_PID, 0);
    vmm_set_mapping(PD_REFERENCED, K_STACK_START+(i << PG_SIZE_BITS), pa, PG_PREM_RW, VMAP_NULL);
  }
  printf_("[MM] Allocated %d pages for stack start at %p\n", K_STACK_SIZE>>PG_SIZE_BITS, K_STACK_START);

}

void _kernel_post_init(){

  /*set malloc heap*/

  assert_msg(kalloc_init() , "heap alloc failed !!\n");

  _lock_reserved_memory();

  acpi_init(_init_mb_info);

  uintptr_t ioapic_addr = acpi_get_context()->madt.ioapic->ioapic_addr;
  pmm_mark_page_occupied(KERNEL_PID, FLOOR(__APIC_BASE_PADDR, PG_SIZE_BITS), 0);
  pmm_mark_page_occupied(KERNEL_PID, FLOOR(ioapic_addr, PG_SIZE_BITS), 0);
  /* APIC_BASE_VADDR = 0xC0000000 , IOAPIC_BASE_VADDR  = oxC0000000 + 2*/
  vmm_set_mapping(PD_REFERENCED, APIC_BASE_VADDR, __APIC_BASE_PADDR, PG_PREM_RW, VMAP_NULL);
  vmm_set_mapping(PD_REFERENCED, IOAPIC_BASE_VADDR, ioapic_addr, PG_PREM_RW, VMAP_NULL);


  apic_init();
  ioapic_init();

  rtc_init();
  timer_primary_init();
  clock_init();
  ps2_kbd_init();

  sched_init();
  syscall_init();

  _unlock_reserved_memory();

  task_1_init();

  schedule();


  spin();

}

void task_1_init(){

  struct task_struct *init_task = alloc_task();
  init_task->created            = 0;
  init_task->parent             = 0;

  init_task->regs = (isr_param){
    .gs     = K_DATA_SEG,
    .fs     = K_DATA_SEG,
    .es     = K_DATA_SEG,
    .ds     = K_DATA_SEG,
    .cs     = K_CODE_SEG,
    .eip    = (void*)task_1_work,
    .ss     = K_DATA_SEG,
    .eflags = cpu_reflags()
  };


  setup_task_page_table(init_task, PD_REFERENCED);

  cpu_lcr3(init_task->page_table);

  vmm_unmount_pg_dir(PD_MOUNT_2);

  asm volatile(
      "movl %%esp, %%ebx\n"
      "movl %1,    %%esp\n"
      "pushf            \n"
      "pushl %2         \n"
      "pushl %3         \n"
      "pushl $0         \n"
      "pushl $0         \n"
      "movl %%esp, %0   \n"
      "movl %%ebx, %%esp\n"
      :"=m"(init_task->regs.esp)
      :"i"(K_STACK_TOP), "r"(init_task->regs.cs), "r"(init_task->regs.eip)
      : "%ebx", "memory"
  );

  commit_task(init_task);

}


void __do_reserved_memory(int lock){
  multiboot_memory_map_t *mmaps = _init_mb_info->mmap_addr;
  uint32_t map_size             = _init_mb_info->mmap_length / sizeof(multiboot_memory_map_t);
  for(uint32_t i=0; i<map_size; ++i){
    multiboot_memory_map_t mmap = mmaps[i];
    uint8_t *pa                 = PG_ALIGN(mmap.addr_low);
    if(mmap.type == MULTIBOOT_MEMORY_AVAILABLE || pa <= MEM_4MB){
      continue;
    }

    uint32_t pg_n = CEIL(mmap.len_low, PG_SIZE_BITS);
    uint32_t j    = 0;
    if(lock){
      for(; j<pg_n; ++j){
        uint32_t addr = pa + (j << PG_SIZE_BITS) ;
        if(addr >= KERNEL_VSTART)break;
        vmm_set_mapping(PD_REFERENCED, addr, addr, PG_PREM_R, VMAP_NULL);
      }
      mmaps[i].len_low = j * PG_SIZE;
    }else{
      for(; j<pg_n; ++j){
        uintptr_t _pa = pa + (j << PG_SIZE_BITS);
        vmm_unset_mapping(PD_REFERENCED, _pa);
      }
    }
  }

}

void _unlock_reserved_memory(){
      __do_reserved_memory(0);
}

void _lock_reserved_memory(){
      __do_reserved_memory(1);
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

  /* mask kernel and 1mb used pages */
  size_t pg_count = (uint32_t)V2P(sym_vaddr(__kernel_end)) >> PG_SIZE_BITS;

  pmm_mark_pages_occupied(KERNEL_PID, 0, pg_count, 0);

  printf_("[MM] Allocated %d pages for kernel.\n", pg_count);

  /* mask vga used and vmap vga */
  uint32_t pg_start = (VGA_BUFFER_PADDR >> PG_SIZE_BITS);
  pg_count          = VGA_BUFFER_SIZE >> PG_SIZE_BITS;
  pmm_mark_pages_occupied(KERNEL_PID, pg_start, pg_count, 0);
 
  for(uint32_t i=0; i<pg_count; ++i){
    vmm_set_mapping(
          PD_REFERENCED,
          (void*)VGA_BUFFER_VADDR + (i << PG_SIZE_BITS) ,
          (void*)VGA_BUFFER_PADDR + (i << PG_SIZE_BITS),
          PG_PREM_URW,
          VMAP_NULL
        );
  }

  /* allow user access(such as some signal code) */
  for(uint32_t i=&__user_text_start; i<&__user_text_end; i+=PG_SIZE){
    vmm_set_mapping(PD_REFERENCED, i, V2P(i), PG_PREM_UR, VMAP_NULL);
  }

  _tty_init((void *)VGA_BUFFER_VADDR);

}



