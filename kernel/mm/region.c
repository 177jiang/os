#include <mm/region.h>
#include <mm/kalloc.h>
#include <mm/valloc.h>
#include <process.h>



void region_add(struct mm_region *regions, uint32_t start, uint32_t end, uint32_t attr){

    struct mm_region *new_region = kmalloc(sizeof(struct mm_region));

    *new_region = (struct mm_region){
        .start = start,
        .end   = end,
        .attr  = attr
    };

    list_append(&regions->head, &new_region->head);

}

void region_release(struct task_struct *task){

    struct mm_region *regions = &task->mm.regions;
    struct mm_region *pos, *n;

    list_for_each(pos, n, &regions->head, head){
        kfree(pos);
    }

    list_init_head(&task->mm.regions.head);
}

struct mm_region *get_region(struct task_struct *task, uint32_t vaddr){

    struct mm_region *region = &task->mm.regions;

    if(!region)return NULL;
    struct mm_region *pos, *n;

    list_for_each(pos, n, &region->head, head) {
        if(pos->start <= vaddr && pos->end > vaddr) {
            return pos;
        }
    }

    return NULL;

}

void region_copy(struct mm_region *from, struct mm_region *to){

    if(!from)return;

    struct mm_region *pos, *n;

    list_for_each(pos, n, &from->head, head){

        region_add(to, pos->start, pos->end, pos->attr);

    }

}







