#ifndef __jyos_fifobuf_h_
#define __jyos_fifobuf_h_

#include <datastructs/mutex.h>

#define FIFO_DIRTY        1


struct fifo_buffer{

    void            *data;

    unsigned int    wpos;
    unsigned int    rpos;

    unsigned int    size;
    unsigned int    flags;

    mutex_t         lock;

};



#endif

