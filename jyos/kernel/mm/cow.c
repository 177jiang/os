#include <mm/vmm.h>


Pysical(void *) vmm_dup_page(pid_t pid, void * paddr){

    void *new_page  = pmm_alloc_page(pid, 0);

    void *a = vmm_map_page_occupy(pid, PG_MOUNT_3, new_page, PG_PREM_RW);
    void *b = vmm_map_page_occupy(pid, PG_MOUNT_4, paddr, PG_PREM_RW);

    asm volatile (
            "movl %1, %%edi\n"
            "movl %2, %%esi\n"
            "rep movsl\n"
            ::"c"(PG_SIZE >> 2), "r"(PG_MOUNT_3), "r"(PG_MOUNT_4)
            :"memory", "%edi", "%esi"
    );


    vmm_unset_mapping(PG_MOUNT_3);
    vmm_unset_mapping(PG_MOUNT_4);

    return new_page;

}
