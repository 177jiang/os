#ifndef __jyos_constant_h_
#define __jyos_constant_h_


#define K_STACK_SIZE            0x100000U
#define K_STACK_START           ((0xFFBFFFFFU - K_STACK_SIZE) + 1)
#define HIGHER_HLF_BASE         0xC0000000UL
#define MEM_1MB                 0x100000UL

#define VGA_BUFFER_VADDR        0xB0000000UL
#define VGA_BUFFER_PADDR        0xB8000UL
#define VGA_BUFFER_SIZE         4096

#define KERNEL_VSTART           0xC0000000UL


#define offset_of(type, member)   ( (size_t) &(((type*)0)->member) )

#define container_of(ptr, type, member) ({                  \
        const typeof( ((type*)0)->member ) *__mptr = (ptr); \
        (type*)((char *)__mptr - offset_of(type, member));  \
        })

#endif
