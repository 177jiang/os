#ifndef __jyos_vmm_h_
#define __jyos_vmm_h_

#include <mm/page.h>
#include <mm/pmm.h>
#include <process.h>
#include <mm/vmm.h>
#include <stdint.h>
#include <stddef.h>

#define VMAP_NULL 0

#define VMAP_IGNORE 1

#define VMAP_NOMAP 2


#define v_page_attr uint32_t

void vmm_init();

Pysical(x86_page_t*) vmm_create_new_pd();

int vmm_set_mapping(uintptr_t mnt, void *va, Pysical(void *) pa, p_page_attr attr, int op);

Pysical(uintptr_t) vmm_unset_mapping(uintptr_t mnt, uintptr_t va);

int vmm_lookup(void *va, v_mapping *mapping);

Pysical(void *) vmm_dup_page(pid_t pid, Pysical(void *paddr) );

void *vmm_mount_pg_dir(uintptr_t mount, void *pde);

void vmm_unmount_pg_dir(uintptr_t mount);

void * vmm_vmap(uintptr_t paddr, size_t n, page_attr attr);

void * vmm_v2p(void *vaddr);


#endif
