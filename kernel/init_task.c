#include <arch/x86/boot/multiboot.h>
#include <mm/page.h>
#include <mm/pmm.h>
#include <mm/kalloc.h>

#include <hal/cpu.h>
#include <hal/rtc.h>
#include <hal/apic.h>
#include <hal/ioapic.h>
#include <hal/acpi/acpi.h>
#include <hal/ahci/ahci.h>
#include <hal/pci.h>

#include <libc/stdio.h>
#include <libc/string.h>

#include <datastructs/jlist.h>

#include <spike.h>
#include <jconsole.h>
#include <mm/vmm.h>
#include <mm/valloc.h>
#include <mm/cake.h>
#include <constant.h>
#include <tty/tty.h>
#include <timer.h>
#include <sched.h>
#include <proc.h>
#include <junistd.h>
#include <syscall.h>

#include <fs/fs.h>
#include <fs/rootfs.h>

#include <stdint.h>
#include <stddef.h>



extern uint8_t __init_hhk_end;
extern int _kernel_main();
void _unlock_reserved_memory();
void _lock_reserved_memory();

/* in user space */
void __USER_SPACE__ __move_to_user_mode(){

  if(!fork()){
    asm("jmp _kernel_main");
  }

  int p;
  if( !(p = fork()) ){
      
      // __test_disk_io();
      __test_readdir();
      _exit(0);
  }else{

      waitpid(p, 0, 0);
      while(1){
        yield();
      }
      spin();
  }


}

void task_1_work(){

  _kernel_post_init();

  init_task_user_space(__current);

  asm volatile(

      "movw   %0,    %%dx \n"
      "movw   %%dx,  %%fs \n"
      "movw   %%dx,  %%gs \n"
      "movw   %%dx,  %%ds \n"
      "movw   %%dx,  %%es \n"

      "pushl  %0 \n"
      "pushl  %1 \n"
      "pushl  %2 \n"
      "pushl  %3 \n"

      "lret      \n"
      :
      :"i"(U_DATA_SEG),
       "i"(U_STACK_TOP & 0xFFFFFFF0),
       "i"(U_CODE_SEG),
       "r"(__move_to_user_mode)
      :"edx", "memory"

  );

  spin();

}

extern multiboot_info_t* _init_mb_info; /* k_init.c */
void _kernel_post_init(){

  _lock_reserved_memory();

  /*set malloc heap*/
  assert_msg(kalloc_init() , "heap alloc failed !!\n");

  cake_init();
  valloc_init();


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
  timer_init(SYS_TIMER_FREQUENCY_HZ);
  clock_init();
  ps2_kbd_init();

  pci_init();
  pci_print_device();

  ahci_init();

  fsm_init();
  vfs_init();
  rootfs_init();

  vfs_mount("/", "rootfs", -1);
  

  syscall_init();

  console_start_flushing();

  cake_stats();

  _unlock_reserved_memory();

  /*release  hhk_init_code*/ /* save 1mb */
  for(uint32_t i=MEM_1MB; i<sym_vaddr(__init_hhk_end); i+=PG_SIZE){
    vmm_unset_mapping(PD_REFERENCED, (void*)i);
    pmm_free_page(KERNEL_PID, i);
  }


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




