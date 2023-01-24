#include <process.h>
#include <mm/page.h>
#include <types.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <timer.h>
#include <spike.h>
#include <syscall.h>
#include <proc.h>
#include <junistd.h>

extern uint8_t __init_hhk_end;
extern int _kernel_main();

void __move_to_user_mode();

void task_1_work(){

  /*release  hhk_init_code*/ /* save 1mb */
  for(uint32_t i=MEM_1MB; i<sym_vaddr(__init_hhk_end); i+=PG_SIZE){
    vmm_unset_mapping(PD_REFERENCED, (void*)i);
    pmm_free_page(KERNEL_PID, i);
  }

  init_task_user_space(__current);

  /* now enable time to avoid competion (in timer_init)*/
  timer_init(SYS_TIMER_FREQUENCY_HZ);

  asm volatile(

      "cli    \n"

      "movw   %0,    %%dx \n"
      "movw   %%dx,  %%fs \n"
      "movw   %%dx,  %%gs \n"
      "movw   %%dx,  %%ds \n"
      "movw   %%dx,  %%es \n"

      "pushl  %0 \n"
      "pushl  %1 \n"
      "pushl  %2 \n"
      "pushl  %3 \n"

      "sti       \n"

      "lret      \n"
      :
      :"i"(U_DATA_SEG),
       "i"(U_STACK_TOP & 0xFFFFFFF0),
       "i"(U_CODE_SEG),
       "i"(__move_to_user_mode)
      :"eax", "memory"
  );

  spin();

}

/* in user space */
void __USER_SPACE__ __move_to_user_mode(){

  if(!fork()){

    asm("jmp _kernel_main");

  }else{
    while(1){
      yield();
    }
  }

  spin();

}






