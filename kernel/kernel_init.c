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
#include <jconsole.h>


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

  uint32_t map_size = _init_mb_info->mmap_length / sizeof (multiboot_memory_map_t);
  setup_mem((multiboot_memory_map_t *)_init_mb_info->mmap_addr, map_size);

}

void _kernel_init() {

  console_init();

  /*alloc stack page for kernel*/
  for(uint32_t i=0; i<(K_STACK_SIZE >> PG_SIZE_BITS); ++i){
    uint32_t pa = pmm_alloc_page(KERNEL_PID, 0);
    vmm_set_mapping(PD_REFERENCED, K_STACK_START+(i << PG_SIZE_BITS), pa, PG_PREM_RW, VMAP_NULL);
  }

  // kprintf_("[MM] Allocated %d pages for stack start at %p\n", K_STACK_SIZE>>PG_SIZE_BITS, K_STACK_START);

  sched_init();

  task_1_init();

}


void task_1_init(){

  struct task_struct *init_task = alloc_task();
  init_task->parent             = init_task;

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

  vmm_unmount_pg_dir(PD_MOUNT_2);

  cpu_lcr3(init_task->page_table);

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

  init_task->state = TASK_RUNNING;

  asm volatile(
      "movl %0, %%eax         \n"
      "jmp   __do_switch_to   \n"
      :
      :"r"(init_task)
  );

  assert_msg(0, "Shoulden't come here !!! \n");

}



void setup_mem(multiboot_memory_map_t *map, uint32_t size){


  for(uint32_t i=0; i<size; ++i){

    multiboot_memory_map_t mmap = map[i];
    if(mmap.type == MULTIBOOT_MEMORY_AVAILABLE){
      uint32_t pgs = (mmap.addr_low + 0x0FFFU) >> PG_SIZE_BITS;
      uint32_t n   =  mmap.len_low >> PG_SIZE_BITS;
      pmm_mark_pages_free(pgs, n);
    }

  }

  /* mask kernel and 1mb used pages */
  size_t pg_count = (uint32_t)V2P(sym_vaddr(__kernel_end)) >> PG_SIZE_BITS;

  pmm_mark_pages_occupied(KERNEL_PID, 0, pg_count, 0);

  /* mask vga used and vmap vga */
  uint32_t pg_start = (VGA_BUFFER_PADDR >> PG_SIZE_BITS);
  pg_count          = VGA_BUFFER_SIZE >> PG_SIZE_BITS;
  pmm_mark_pages_occupied(KERNEL_PID, pg_start, pg_count, PPG_LOCKED);
 

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



