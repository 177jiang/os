#ifndef __jyos_mm_h_
#define __jyos_mm_h_

#include <datastructs/jlist.h>
#include <datastructs/mutex.h>

/**
 * @brief 私有区域，该区域中的页无法进行任何形式的共享。
 * 
 */
#define REGION_PRIVATE      0x0

/**
 * @brief 读共享区域，该区域中的页可以被两个进程之间读共享，但任何写操作须应用Copy-On-Write
 * 
 */
#define REGION_RSHARED      0x1

/**
 * @brief 写共享区域，该区域中的页可以被两个进程之间读共享，任何的写操作无需执行Copy-On-Write
 * 
 */
#define REGION_WSHARED      0x2

#define REGION_TYPE_CODE    (0)
#define REGION_TYPE_DATA    (1 << 2)

typedef struct {

    uint8_t *start;
    uint8_t *end;
    uint8_t *max_addr;
    mutex_t lock;

}heap_context_t;

struct mm_region {
    struct list_header  *head;
    void                *start;
    void                *end;
    unsigned int         attr;
};



#endif
