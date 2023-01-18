#ifndef __jyos_console_h_
#define __jyos_console_h_

#include <datastructs/fifobuf.h>
#include <timer.h>

struct console{

    struct timer *flush_timer;
    struct fifo_buffer buffer;

    unsigned int erd_pos;
    unsigned int lines;
    unsigned int chars;
};



#endif
