#ifndef __jyos_page_h_
#define __jyos_page_h_

#include <stdint.h>
#include <constant.h>

#define sym_vaddr(sym)    (&sym)

#define Pysical(x)   x

#define PG_SIZE_BITS                12
#define PG_SIZE                     (1 << PG_SIZE_BITS)

#define PG_MAX_ENTRIES              1024U
#define PG_LAST_TABLE               PG_MAX_ENTRIES - 1
#define PD_LAST_INDEX               PG_MAX_ENTRIES - 1

#define PG_FIRST_TABLE              0

#define P2V(paddr)          ((uintptr_t)(paddr)  +  HIGHER_HLF_BASE)
#define V2P(vaddr)          ((uintptr_t)(vaddr)  -  HIGHER_HLF_BASE)

#define PG_ALIGN(addr)      ((uintptr_t)(addr)   & 0xFFFFF000UL)

#define PD_INDEX(vaddr)     (uint32_t)(((uintptr_t)(vaddr) & 0xFFC00000UL) >> 22)
#define PT_INDEX(vaddr)     (uint32_t)(((uintptr_t)(vaddr) & 0x003FF000UL) >> 12)

#define PG_OFFSET(vaddr)    ((uintptr_t)(vaddr)  & 0x00000FFFUL)

#define GET_PT_ADDR(pde)    PG_ALIGN(pde)
#define GET_PG_ADDR(pte)    PG_ALIGN(pte)

#define PG_DIRTY(pte)           (((pte) & (1 << 6)) >> 6)
#define PG_ACCESSED(pte)        (((pte) & (1 << 5)) >> 5)

#define IS_CACHED(entry)    ((entry & 0x1))

#define PG_PRESENT              (0x1)
#define PG_WRITE                (0x1 << 1)
#define PG_ALLOW_USER           (0x1 << 2)
#define PG_WRITE_THROUGH        (1 << 3)
#define PG_DISABLE_CACHE        (1 << 4)
#define PG_PDE_4MB              (1 << 7)

#define PDE(flags, pt_addr)     (PG_ALIGN(pt_addr) | (((flags) | PG_WRITE_THROUGH)& 0xfff))
#define PTE(flags, pg_addr)     (PG_ALIGN(pg_addr) | ((flags) & 0xfff))

#define V_ADDR(pd, pt, offset)  ((pd) << 22 | (pt) << 12 | (offset))
#define P_ADDR(ppn, offset)     ((ppn << 12) | (offset))

#define PG_PREM_R              PG_PRESENT
#define PG_PREM_RW             PG_PRESENT | PG_WRITE
#define PG_PREM_UR             PG_PRESENT | PG_ALLOW_USER
#define PG_PREM_URW            PG_PRESENT | PG_WRITE | PG_ALLOW_USER
// 用于对PD进行循环映射，因为我们可能需要对PD进行频繁操作，我们在这里禁用TLB缓存
#define T_SELF_REF_PERM        PG_PREM_RW | PG_DISABLE_CACHE | PG_WRITE_THROUGH

#define PTE_NULL              (0)

#define PG_ENTRY_FLAGS(entry)   ((entry) & 0xFFFU)
#define PG_ENTRY_ADDR(entry)   ((entry) & ~0xFFFU)

#define HAS_FLAGS(entry, flags)             ((PG_ENTRY_FLAGS(entry) & (flags)) == flags)
#define CONTAINS_FLAGS(entry, flags)        (PG_ENTRY_FLAGS(entry) & (flags))

// 页目录的虚拟基地址，可以用来访问到各个PDE
#define PD_BASE_VADDR                0xFFFFF000U

// 页表的虚拟基地址，可以用来访问到各个PTE
#define PT_BASE_VADDR                 0xFFC00000U

// 用来获取特定的页表的虚拟地址
#define PT_VADDR(pd_offset)           ((PT_BASE_VADDR) | ((pd_offset) << 12))

typedef unsigned int ptd_t;
typedef unsigned int pt_t;
typedef unsigned int pt_attr;
typedef uint32_t x86_pte_t;

typedef struct {
  x86_pte_t entry[PG_MAX_ENTRIES];
} __attribute__((packed)) x86_page_t ;

typedef  struct {

  uint32_t    va;
  uint32_t    p_index;
  uint32_t    p_addr;
  uint16_t    flags;
  x86_pte_t   *pte;
  
} v_mapping;

extern void __pg_mount_point;
extern void __pd_mount_point;



#define MEM_4MB             (MEM_1MB << 2)
#define MNT_PG_BASE         ((uint32_t)sym_vaddr(__pg_mount_point))
#define MNT_PD_BASE         ((uint32_t)sym_vaddr(__pd_mount_point))

#define PD_MOUNT_2      (MNT_PD_BASE)
#define PD_MOUNT_1      (PD_MOUNT_2 + MEM_4MB)


#define PG_MOUNT_1      (MNT_PG_BASE)
#define PG_MOUNT_2      (MNT_PG_BASE + PG_SIZE)
#define PG_MOUNT_3      (MNT_PG_BASE + PG_SIZE + PG_SIZE)
#define PG_MOUNT_4      (MNT_PG_BASE + PG_SIZE + PG_SIZE + PG_SIZE)

#define PD_REFERENCED    0xFFC00000U

#define PG_HIG_MSK    0xFFC00000
#define PG_HIG(vaddr) (PG_HIG_MSK & vaddr)

#define PG_MID_MSk    0x003FF000
#define PG_MID(vaddr) (PG_MID_MSk & vaddr)

#define __CURRENT_PTE(vaddr) \
  ((x86_page_t *)(PD_MOUNT_1 | (PG_HIG(vaddr)>>10)))->entry + ((PG_MID(vaddr)) >> 12)

#define __MOUNTED_PTE(mnt, vaddr) \
  ((x86_page_t *)((mnt) | (PG_HIG(vaddr)>>10)))->entry + ((PG_MID(vaddr)) >> 12)

#define __MOUNTED_PGDIR(mnt)  \
  ((x86_page_t*)( (mnt) | (1023 << 12) ))

#define __MOUNTED_PGTABLE(mnt, va) \
  ((x86_page_t*)( (mnt) | (PD_INDEX(va) << 12) ))

#endif





