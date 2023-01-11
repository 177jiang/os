#ifndef __jyos_constant_h_
#define __jyos_constant_h_


#define KERNEL_VSTART           0xC0000000

#define APIC_BASE_VADDR         KERNEL_VSTART
#define IOAPIC_BASE_VADDR       APIC_BASE_VADDR + (1 << 12)
#define VGA_BUFFER_VADDR        IOAPIC_BASE_VADDR + (1 << 12)

#define VGA_BUFFER_PADDR        0xB8000
#define VGA_BUFFER_SIZE         4096


#define K_STACK_SIZE            (64 << 10)
#define K_STACK_START           ((0x3FFFFFU - K_STACK_SIZE) + 1)
#define K_STACK_TOP             (0x3FFFF0U)


#define HIGHER_HLF_BASE         0xC0000000

#define MEM_1MB                 0x100000
#define MEM_4MB                 0x400000

#define K_CODE_MAX_SIZE         MEM_4MB
#define K_HEAP_START            (KERNEL_VSTART + K_CODE_MAX_SIZE)
#define K_HEAP_SIZE_MB          (256)

#define TASK_TABLE_SIZE_MB      (4)
#define TASK_TABLE_START        (K_HEAP_START + (K_HEAP_SIZE_MB * MEM_1MB))



#define K_CODE_SEG              0x08
#define K_DATA_SEG              0x10
#define U_CODE_SEG              0x1B
#define U_DATA_SEG              0x23
#define TSS_SEG                 0x28

#define USER_START              0x400000
#define U_STACK_SIZE            0x100000
#define U_STACK_TOP             0x9fffffff
#define U_STACK_END             (U_STACK_TOP - U_STACK_SIZE + 1)

#define U_MMAP_AREA             0x4D000000


#ifndef __ASM_S_
#define offset_of(type, member)   ( (size_t) &(((type*)0)->member) )

#define container_of(ptr, type, member) ({                  \
        const typeof( ((type*)0)->member ) *__mptr = (ptr); \
        (type*)((char *)__mptr - offset_of(type, member));  \
        })

#endif

#endif
