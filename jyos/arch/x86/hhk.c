#include <arch/x86/boot/multiboot.h>
#include <libc/stdio.h>
#include <libc/string.h>
#include <jyos/constant.h>
#include <jyos/mm/page.h>
#include <stdint.h>

#define PT_ADDR(ptd, index)                       ((ptd_t*)ptd + ((index + 1) <<  10) )

#define SET_PDE(ptd, index, pde)                 *((ptd_t*)ptd + index) = pde;

#define SET_PTE(ptd, page_index, page_offset, pte)       *((ptd_t*)PT_ADDR(ptd, page_index) + page_offset) = pte;

#define sym_val(sym)                                 (uintptr_t)(&sym)

#define KERNEL_PAGE_COUNT           ((sym_val(__kernel_end) - sym_val(__kernel_start) + 0x1000 - 1) >> 12);
#define HHK_PAGE_COUNT              ((sym_val(__init_hhk_end) - 0x100000 + 0x1000 - 1) >> 12)

// use table #1
#define PG_TABLE_IDENTITY_INDEX           0

// use table #2-4
// hence the max size of kernel is 8MiB
#define PG_TABLE_KERNEL_INDEX             1

// use table #5
#define PG_TABLE_STACK_INDEX              9


extern uint8_t __kernel_start;
extern uint8_t __kernel_end;
extern uint8_t __init_hhk_end;
extern uint8_t _k_stack;

void _init_page_table(ptd_t *ptd){

  /*first page table in page dir*/
  SET_PDE(
      ptd,
      0,
      PDE(PG_PRESENT, ptd + PG_MAX_ENTRIES )
  );

  /*1mb + hhk_init  identity map*/
  for(uint32_t i=0; i<256 + HHK_PAGE_COUNT; ++i){

    SET_PTE(
        ptd,
        PG_TABLE_IDENTITY_INDEX,
        i,
        PTE(PG_PREM_RW, (i << 12) )
    );

  }

  /* map kernel space 0xc0000000 */
  uint32_t kernel_pde_index   = PD_INDEX(sym_val(__kernel_start));
  uint32_t kernel_pte_index   = PT_INDEX(sym_val(__kernel_start));
  uint32_t kernel_page_counts = KERNEL_PAGE_COUNT;
  uintptr_t kernel_paddr      = V2P(sym_val(__kernel_start));
  uint32_t dir_counts         = PG_TABLE_STACK_INDEX - PG_TABLE_KERNEL_INDEX;

  if( kernel_page_counts > (dir_counts << 10 ) ){
    while(1);
  }

  for(uint32_t i=0; i<dir_counts; ++i){

    SET_PDE(
        ptd,
        kernel_pde_index + i,
        PDE(
          PG_PREM_RW,
          PT_ADDR(ptd, PG_TABLE_KERNEL_INDEX + i)
          )
    );

  }

  for(uint32_t i=0; i<kernel_page_counts; ++i){

    SET_PTE(
        ptd,
        PG_TABLE_KERNEL_INDEX ,
        kernel_pte_index + i,
        PTE(
          PG_PREM_RW,
          (kernel_paddr + (i << 12))
          )
    );

  }

/* syscle */
  SET_PDE(
      ptd,
      1023,
      PDE(T_SELF_REF_PERM, ptd)
  );

}


void _save_multiboot_info(multiboot_info_t *info, char *dest){

  char *addr = (char *)info;

  size_t len   = sizeof(multiboot_info_t);

  for(uint32_t i=0; i<len ; ++i)*(dest + i) = *(addr + i);
  
  ((multiboot_info_t *) dest)->mmap_addr = (uintptr_t)dest + len;

  addr = (char *)info->mmap_addr;

  for(uint32_t i=0; i<info->mmap_length; ++i)*(dest + len++) = *(addr + i);

  if(present(info->flags, MULTIBOOT_INFO_DRIVE_INFO)) {

    ((multiboot_info_t *) dest)->drives_addr = (uintptr_t)dest+ len;

    addr = (char *)info->drives_addr;
    for(uint32_t i=0; i<info->drives_length; ++i)*(dest + len++) = *(addr + i);

  }

}


void _hhk_init(ptd_t *ptd, uint32_t kpg_size){

  char *pg = (char *)ptd;
  for(uint32_t i=0; i<kpg_size; ++i)*(pg + i) = 0;

  _init_page_table(ptd);

}
