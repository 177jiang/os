#include <mm/vmm.h>


Pysical(void *) vmm_dup_page(void * vaddr){

    void *new_page  = pmm_alloc_page(KERNEL_PID, 0);
    vmm_map_page_occupy(KERNEL_PID, PG_MOUNT_3, new_page, PG_PREM_RW);

    asm volatile (
            "movl %1, %%edi\n"
            "rep movsl\n"
            ::"c"(PG_SIZE >> 2), "r"(PG_MOUNT_3), "S"((uint32_t*)vaddr)
            :"memory", "%edi"
    );

    vmm_unset_mapping(PG_MOUNT_3);
    return new_page;

}
