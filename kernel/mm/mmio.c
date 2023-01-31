#include <mm/pmm.h>
#include <mm/vmm.h>
#include <mm/mmio.h>

void *ioremap(uintptr_t paddr, unsigned int size){

  return vmm_vmap(paddr, size, (PG_PREM_RW | PG_DISABLE_CACHE));

}
