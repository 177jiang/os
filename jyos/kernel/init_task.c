#include <process.h>
#include <mm/page.h>
#include <types.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <timer.h>

extern uint8_t __init_hhk_end;
extern int _kernel_main();

void task_1_work(){

  /* now enable time to avoid competion (in timer_init)*/
  timer_init(SYS_TIMER_FREQUENCY_HZ);

  /*release  hhk_init_code*/ /* save 1mb */
  for(uint32_t i=MEM_1MB; i<sym_vaddr(__init_hhk_end); i+=PG_SIZE){
    vmm_unset_mapping(PD_REFERENCED, (void*)i);
    pmm_free_page(KERNEL_PID, i);
  }

  init_task_user_space(__current);

  _kernel_main();

}
