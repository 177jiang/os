
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <spike.h>
#include <libc/string.h>
#include <hal/cpu.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

void vmm_init() { }

#define RECURSION_MAP_INDEX   (PG_MAX_ENTRIES - 1)

Pysical(x86_page_t *) vmm_create_new_pd(){

  x86_page_t * page_dir = (x86_page_t *)pmm_alloc_page(0, PP_FGPERSIST);

  for(uint32_t i=0; i<PG_MAX_ENTRIES; ++i){
    page_dir->entry[i] = 0;
  }

  page_dir->entry[RECURSION_MAP_INDEX] = PDE(T_SELF_REF_PERM, page_dir);

  return page_dir;

}

int vmm_set_mapping(uintptr_t mnt, void *va, Pysical(void *) pa, p_page_attr attr, int op){

    assert_msg((uint32_t)pa % PG_SIZE==0, "Paddr must be align 4k\n");
    assert_msg(attr <= 128, "attr set error\n");

    uintptr_t d_index      = PD_INDEX(va);
    uintptr_t t_index      = PT_INDEX(va);
    x86_page_t *page_dir   = __MOUNTED_PGDIR(mnt);
    x86_page_t *page_table = __MOUNTED_PGTABLE(mnt, va);

    if(!page_dir->entry[d_index]){
      x86_page_t *new_page_tb = pmm_alloc_page(KERNEL_PID, PP_FGPERSIST);

      if(!new_page_tb){
        return 0;
      }

      page_dir->entry[d_index] = PDE(attr | PG_WRITE | PG_PRESENT, new_page_tb);

      memset((void*)page_table, 0, PG_SIZE);

    }else{

      x86_pte_t pte = page_table->entry[t_index];

      if(pte && (op & VMAP_IGNORE)){
        return 1;
      }

    }

    if(mnt == PD_REFERENCED){
      cpu_invplg(va);
    }

    if((op & VMAP_NOMAP)){
      return 1;
    }

    page_table->entry[t_index] = PTE(attr, pa);

    return 1;

}


uintptr_t vmm_unset_mapping(uintptr_t mnt, uintptr_t va){

  assert_msg(!((uint32_t)va&0xFFF), "vmm unset must align 4K\n");

  uint32_t d_index = PD_INDEX(va);
  uint32_t t_index = PT_INDEX(va);

  if(d_index == PD_LAST_INDEX){
    return 0;
  }

  x86_page_t *page_dir = __MOUNTED_PGDIR(mnt);
  x86_pte_t  pde       = page_dir->entry[d_index];
  if(pde){

    x86_page_t *page_table = __MOUNTED_PGTABLE(mnt, va);

    x86_pte_t   pte        = page_table->entry[t_index];

    cpu_invplg(va);

    page_table->entry[t_index] = PTE_NULL;

    return PG_ENTRY_ADDR(pte);

  }

  return 0;

}


int vmm_lookup(void *va, v_mapping *mapping){

  uint32_t d_index = PD_INDEX(va);
  uint32_t t_index = PT_INDEX(va);

  x86_page_t * page_dir = (x86_page_t *)PD_BASE_VADDR;
  x86_pte_t pde         = page_dir->entry[d_index];

  if(pde){

    x86_pte_t *pte =
      ((x86_page_t *)PT_VADDR(d_index))->entry + t_index;

    if(pte){
      mapping->flags   = PG_ENTRY_FLAGS(*pte);
      mapping->p_addr  = PG_ENTRY_ADDR(*pte);
      mapping->p_index = mapping->p_addr >> PG_SIZE_BITS;
      mapping->pte     = pte;
      mapping->va      = va;
      return 1;
    }

  }
  return 0;
}


void *vmm_mount_pg_dir(uintptr_t mount, void *pde){

  x86_page_t *page_dir         = (x86_page_t *)PD_BASE_VADDR;

  page_dir->entry[mount >> 22] = PDE(T_SELF_REF_PERM, pde);

  cpu_invplg(mount);

  return mount;
}

void vmm_unmount_pg_dir(uint32_t mount){


  x86_page_t *page_dir            = (x86_page_t *)PD_BASE_VADDR;

  page_dir->entry[mount>> 22] = 0;

  cpu_invplg(mount);

}













