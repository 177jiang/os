#ifndef __jyos_constant_h_
#define __jyos_constant_h_


#define K_STACK_SIZE            (64 << 10)
#define K_STACK_START            ((0xFFBFFFFFU - K_STACK_SIZE) + 1)
#define K_STACK_TOP              (0xFFBFFFF0)
#define HIGHER_HLF_BASE         0xC0000000
#define MEM_1MB                 0x100000

#define VGA_BUFFER_VADDR        0xB0000000
#define VGA_BUFFER_PADDR        0xB8000
#define VGA_BUFFER_SIZE         4096

#define KERNEL_VSTART           0xC0000000

#define K_CODE_SEG              0x08
#define K_DATA_SEG              0x10
#define U_CODE_SEG              0x1B
#define U_DATA_SEG              0x23
#define TSS_SEG                 0x28


#ifndef __ASM_S_
#define offset_of(type, member)   ( (size_t) &(((type*)0)->member) )

#define container_of(ptr, type, member) ({                  \
        const typeof( ((type*)0)->member ) *__mptr = (ptr); \
        (type*)((char *)__mptr - offset_of(type, member));  \
        })

#endif

#endif
