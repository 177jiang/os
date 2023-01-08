#ifndef __jyos_vmm_h_
#define __jyos_vmm_h_

#include <mm/page.h>
#include <mm/pmm.h>
#include <process.h>
#include <mm/vmm.h>
#include <stdint.h>
#include <stddef.h>

#define v_page_attr uint32_t

void vmm_init();

Pysical(x86_page_t*) vmm_create_new_pd();

void * vmm_map_page(pid_t pid, void *va, Pysical(void *pa), v_page_attr attr);

int vmm_map_pages(pid_t pid, void *va, Pysical(void *pa), size_t pgs, v_page_attr attr);

void * vmm_map_page_occupy(pid_t pid, void *va, Pysical(void * pa), v_page_attr attr);

void *vmm_alloc_page(pid_t pid, void *va, void **pa, v_page_attr vattr, p_page_attr pattr);

int vmm_alloc_pages(pid_t pid, void *va, size_t sz, v_page_attr vattr, p_page_attr pattr);

int vmm_set_mapping(pid_t pid, void *va, Pysical(void *) pa, p_page_attr attr);

void vmm_unmap_page(pid_t pid, void *va);

void vmm_unset_mapping(void *va);

Pysical(void *) vmm_v2p(void *va);

v_mapping vmm_lookup(void *va);

Pysical(void *) vmm_dup_page(pid_t pid, Pysical(void *paddr) );

void *vmm_mount_pg_dir(uintptr_t mount, void *pde);

void vmm_unmount_pg_dir(uintptr_t mount);


#endif
