#ifndef __jyos_region_h_
#define __jyos_region_h_

#include <mm/mm.h>
#include <process.h>

void region_add(struct task_struct *task, uint32_t start, uint32_t end, uint32_t attr);
void region_release(struct task_struct *task);

struct mm_region *get_region(struct task_struct *task, uint32_t vaddr);

#endif
