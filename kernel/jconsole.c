
#include <libc/string.h>

#include <mm/pmm.h>
#include <mm/vmm.h>

#include <tty/console.h>
#include <tty/tty.h>

#include <constant.h>
#include <jconsole.h>



static struct console k_console;

volatile int can_flush = 0;

void console_init(){

    memset(&k_console, 0, sizeof (k_console));

    k_console.buffer.data = VGA_BUFFER_VADDR + PG_SIZE;
    k_console.buffer.size = PG_SIZE << 1;
    k_console.flush_timer =  0;

    mutex_init(&k_console.buffer.lock);

    for(uint32_t i=0; i<(k_console.buffer.size); i += PG_SIZE){

        char *pa = pmm_alloc_page(KERNEL_PID, 0);
        vmm_set_mapping(
                PD_REFERENCED,
                (char*)k_console.buffer.data + i,
                pa,
                PG_PREM_URW,
                0
        );
    }

    memset(k_console.buffer.data, 0, k_console.buffer.size);

}

void console_view_up(){

    struct fifo_buffer *buffer = &k_console.buffer;

    mutex_lock(&buffer->lock);

    size_t p = k_console.erd_pos - 2;

    while(p < k_console.erd_pos &&
          p != buffer->wpos && ((char*)buffer->data)[p] != '\n'){
        --p;
    }

    if(++p > k_console.erd_pos) p = 0;

    k_console.erd_pos = p;
    buffer->flags    |= FIFO_DIRTY;

    mutex_unlock(&buffer->lock);

}

size_t __console_next_line(size_t pos){

    while(pos != k_console.buffer.wpos &&
            ((char*)k_console.buffer.data)[pos] != '\n'){
        ++pos;
    }
    return ++pos;

}

void console_view_down(){

    struct fifo_buffer *buffer = &k_console.buffer;

    mutex_lock(&buffer->lock);

    k_console.erd_pos = __console_next_line(k_console.erd_pos);
    buffer->flags    |= FIFO_DIRTY;

    mutex_unlock(&buffer->lock);

}

void __flush_to_tty(void *arg){

    if(mutex_on_hold(&k_console.buffer.lock)){
        return ;
    }

    if(!(k_console.buffer.flags & FIFO_DIRTY)){
        return ;
    }

    tty_flush_buffer(
            k_console.buffer.data,
            k_console.erd_pos,
            k_console.buffer.wpos,
            k_console.buffer.size
    );

    k_console.buffer.flags &= ~FIFO_DIRTY;

}



void console_write(struct console *console, char *data, size_t size){

    mutex_lock(&console->buffer.lock);

    char *buffer     = console->buffer.data;
    unsigned int  wp = console->buffer.wpos;
    unsigned int  rp = console->buffer.rpos;

    int lines = 0, si = size, i=0;
    char c;

    while(i < size){
        c = *(data + i);
        buffer[(wp + i) % console->buffer.size] = c;
        lines += (c == '\n');
        ++i;
    }

    if(console->lines > TTY_HEIGHT && lines){
        console->buffer.rpos =
            __console_next_line((si + rp) % console->buffer.size);
    }

    unsigned int n_wp = (wp + si) % console->buffer.size;

    if(n_wp < (wp + si) && n_wp > rp){
        console->buffer.rpos = n_wp;
    }

    console->buffer.wpos    = n_wp;
    console->erd_pos        = console->buffer.rpos;
    console->lines         += lines;
    console->buffer.flags  |= FIFO_DIRTY;

    mutex_unlock(&console->buffer.lock);

}

void console_write_str(const char *str, size_t size){

    console_write(&k_console, str, size);

}

void console_write_char(const char c){

    console_write(&k_console, &c, 1);

}

void console_start_flushing(){

    struct timer *timer =
        timer_run_ms(20, __flush_to_tty, NULL, TIMER_MODE_PERIODIC);

    k_console.flush_timer = timer;

}




