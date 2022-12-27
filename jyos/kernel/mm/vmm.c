
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

int __vmm_map_page
  (pid_t pid, uint32_t d_index, uint32_t t_index, Pysical(void *p_addr), v_page_attr attr, int foced){

    if(d_index == RECURSION_MAP_INDEX)return 0;

    x86_page_t * page_dir   = (x86_page_t *) PD_BASE_VADDR;
    x86_page_t * page_table = (x86_page_t *) PT_VADDR(d_index);


    if(!page_dir->entry[d_index]){

        x86_page_t * new_page_table = pmm_alloc_page(pid, PP_FGPERSIST);

        if(new_page_table == 0)return 0;

        page_dir->entry[d_index] = PDE(attr | PG_WRITE, new_page_table);

        memset((void *)page_table, 0, PG_SIZE);

    }

    x86_pte_t pte = page_table->entry[t_index];

    if(pte && !foced) return 0;

    if(HAS_FLAGS(attr, PG_PRESENT)){
      pmm_ref_page(pid, p_addr);
    }

    page_table->entry[t_index] = PTE(attr, p_addr);

    return 1;
}

void * vmm_map_page(pid_t pid, void *va, Pysical(void *pa), v_page_attr attr){

  if(!va || !pa)return 0;

  assert_msg(((uint32_t)va & 0xFFFU)==0, "page must align 4k");
  assert_msg(((uint32_t)va & 0xFFFU)==0, "page must align 4k");

  uint32_t d_index = PD_INDEX(va);
  uint32_t t_index = PT_INDEX(va);

  x86_page_t * page_dir    = (x86_page_t *) PD_BASE_VADDR;

  x86_pte_t  pde = page_dir->entry[d_index];

  while(pde && d_index < PG_MAX_ENTRIES - 1){

    if(t_index == PG_MAX_ENTRIES){
      if(++d_index == PG_MAX_ENTRIES)break;
      t_index = 0;
      pde = page_dir->entry[d_index];
    }

    if(__vmm_map_page(pid, d_index, t_index, pa, attr, 0)){
      return (void *)V_ADDR(d_index, t_index, PG_OFFSET(va));
    }

    ++t_index;

  }

  if(d_index == PG_MAX_ENTRIES - 1)return 0;

  if(!__vmm_map_page(pid, d_index, t_index, pa, attr, 0)){
    return 0;
  }

  return (void *)V_ADDR(d_index, t_index, PG_OFFSET(va));

}

void * vmm_map_page_occupy(pid_t pid, void *va, Pysical(void * pa), v_page_attr attr){

  if(!pa || !va)return 0;
  
  assert_msg(((uint32_t)va & 0xFFFU)==0, "page must align 4k");
  assert_msg(((uint32_t)va & 0xFFFU)==0, "page must align 4k");

  uint32_t d_index = PD_INDEX(va);
  uint32_t t_index = PT_INDEX(va);

  if(!__vmm_map_page(pid, d_index, t_index, pa, attr, 1)){
    return 0;
  }

  cpu_invplg(va);

  return va;

}

void *vmm_alloc_page(pid_t pid, void *va, void **pa, v_page_attr vattr, p_page_attr pattr){

  void *page  = pmm_alloc_page(pid, pattr);
  va = vmm_map_page(pid, va, page, vattr);
      
  if(!va){
    pmm_free_page(pid, page);
  }

  if(pa) *pa = page;

  return va;
}

int vmm_alloc_pages(pid_t pid, void *va, size_t sz, v_page_attr vattr, p_page_attr pattr){

    uint32_t page_n = sz >> PG_SIZE_BITS;
    char *v_addr    = va;

    for(uint32_t i=0; i<page_n; ++i, v_addr += PG_SIZE){

      void *page = pmm_alloc_page(pid, pattr);
      uint32_t d_index = PD_INDEX(v_addr);
      uint32_t t_index = PT_INDEX(v_addr);

      if(!page ||
          !__vmm_map_page(pid, d_index, t_index, page, vattr, 0)){

        v_addr = va;
        for(uint32_t j=0; j<i; ++j, v_addr += PG_SIZE){
          vmm_unmap_page(pid, v_addr);
        }

        return 0;

      }

    }

    return 1;

}

int vmm_map_pages(pid_t pid, void *va, Pysical(void *pa), size_t pgs, v_page_attr attr){

  if(!va || !pa)return 0;

  assert_msg(((uint32_t)va & 0xFFFU)==0, "page must align 4k");
  assert_msg(((uint32_t)va & 0xFFFU)==0, "page must align 4k");

  char *v_addr = va;
  char *p_addr = pa;
  for(uint32_t i=0; i<pgs; ++i, v_addr+=PG_SIZE, p_addr+=PG_SIZE){
    uint32_t d_index = PD_INDEX(v_addr);
    uint32_t t_index = PT_INDEX(v_addr);

    if(!__vmm_map_page(pid, d_index, t_index, pa, attr, 1)){
      v_addr = va;
      for(uint32_t j=0; j<i; ++j,v_addr += PG_SIZE){
        vmm_unmap_page(pid, v_addr);
        return 0;
      }
    }

  }

  return 1;

}

int vmm_set_mapping(pid_t pid, void *va, Pysical(void *) pa, p_page_attr attr){

  assert_msg(((uint32_t)va & 0xFFFU)==0, "page must align 4k");
  assert_msg(((uint32_t)va & 0xFFFU)==0, "page must align 4k");

  uint32_t d_index = PD_INDEX(va);
  uint32_t t_index = PT_INDEX(va);

  if(d_index == RECURSION_MAP_INDEX)return 0;

  return __vmm_map_page(pid, d_index, t_index, pa, attr, 0);

}

void __vmm_unmap_page(pid_t pid, void *va, int free_style){

  uint32_t d_index = PD_INDEX(va);
  uint32_t t_index = PT_INDEX(va);

  if(d_index == RECURSION_MAP_INDEX)return ;
  
  x86_page_t * page_dir = (x86_page_t *)PD_BASE_VADDR;
  x86_pte_t pde = page_dir->entry[d_index];

  if(pde){

    x86_page_t *page_table = (x86_page_t *)PT_VADDR(d_index);
    x86_pte_t pte = page_table->entry[t_index];

    if(IS_CACHED(pte) && free_style){
      pmm_free_page(pid, (void *)pte);
    }

    cpu_invplg(va);
    page_table->entry[t_index] = 0;

  }

}

void vmm_unset_mapping(void *va){
  __vmm_unmap_page(0, va, 0);
}

void vmm_unmap_page(pid_t pid, void *va){
  __vmm_unmap_page(pid, va, 1);
}

v_mapping vmm_lookup(void *va){

  uint32_t d_index = PD_INDEX(va);
  uint32_t t_index = PT_INDEX(va);

  x86_page_t * page_dir = (x86_page_t *)PD_BASE_VADDR;
  x86_pte_t pde = page_dir->entry[d_index];
  
  v_mapping mapping = {
    .flags = 0,
    .p_addr = 0,
    .p_index = 0
  };

  if(pde){

    x86_pte_t *pte =
      ((x86_page_t *)PT_VADDR(d_index))->entry + t_index;

    if(pte){
      mapping.flags = PG_ENTRY_FLAGS(*pte);
      mapping.p_addr = PG_ENTRY_ADDR(*pte);
      mapping.p_index = mapping.p_addr >> PG_SIZE_BITS;
      mapping.pte = pte;
    }

  }
  return mapping;
}

Pysical(void *) vmm_v2p(void *va){
  return (void *)vmm_lookup(va).p_addr;
}


Pysical(void *) vmm_dup_page(void *va){
  return 0; return va;
}




