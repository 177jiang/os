#ifndef __jyos_jconsole_h_
#define __jyos_jconsole_h_

void console_init();

void console_write_str(const char * str, size_t size);

void console_write_char(const char c);

void console_view_up();

void console_view_down();

void console_start_flushing();


#endif
