#include <mm/vmm.h>


Pysical(void *) vmm_dup_page(pid_t pid, void * paddr){

    void *new_page = pmm_alloc_page(pid, 0);
    vmm_set_mapping(PD_REFERENCED, PG_MOUNT_3, new_page, PG_PREM_RW, VMAP_NULL);
    vmm_set_mapping(PD_REFERENCED, PG_MOUNT_4, paddr, PG_PREM_RW, VMAP_NULL);

    asm volatile (
            "movl %1, %%edi\n"
            "movl %2, %%esi\n"
            "rep movsl\n"
            ::"c"(PG_SIZE >> 2), "r"(PG_MOUNT_3), "r"(PG_MOUNT_4)
            :"memory", "%edi", "%esi"
    );

    vmm_unset_mapping(PD_REFERENCED, PG_MOUNT_3);
    vmm_unset_mapping(PD_REFERENCED, PG_MOUNT_4);

    return new_page;

}
