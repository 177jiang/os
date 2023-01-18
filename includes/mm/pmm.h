#ifndef __jyos_pmm_h_
#define __jyos_pmm_h_

#include <stdint.h>
#include <stddef.h>
#include <process.h>


#define PPG_PERSIST         0x1
#define PPG_LOCKED          0x2

#define p_page_attr         uint32_t 

#define START_PAGE          1

#define QEMU_MEM_SIZE       (512 << 20)

// #define MEM_MEP_MAX_SIZE (QEMU_MEM_SIZE >> 12)
#define MEM_MEP_MAX_SIZE    (1 << 20)

#define PP_FGPERSIST        0x1

struct p_page{
  pid_t       owner;
  uint32_t    ref_counts;
  p_page_attr attr;
};

void pmm_mark_page_free(uint32_t ppn);
void pmm_mark_pages_free(uint32_t start_pnn, uint32_t n);

void pmm_mark_page_occupied(pid_t owner, uint32_t ppn, p_page_attr attr);

void pmm_mark_pages_occupied
  (pid_t owner, uint32_t start_pnn, uint32_t n, p_page_attr attr);

void pmm_init(uint32_t mem_upper_limit);

void *pmm_alloc_page(pid_t owner, p_page_attr attr);
void *pmm_alloc_pages(pid_t ower, size_t n, p_page_attr attr);

int pmm_free_page(pid_t, void *pg);

int pmm_ref_page(pid_t owner, void *page);

struct p_page  *pmm_query(void *p_addr);



#endif
