#ifndef __jyos_tty_h_
#define __jyos_tty_h_

#include <stddef.h>
#include <stdint.h>

#define F_BLACK                 30
#define F_RED                   31
#define F_GREEN                 32
#define F_YELLOW                33
#define F_BLUE                  34
#define F_PURPLE                35
#define F_DGREEN                36
#define F_WHITE                 37
#define F_RESTORE               39

#define B_BLACK                 40
#define B_RED                   41
#define B_GREEN                 42
#define B_YELLOW                43
#define B_BLUE                  44
#define B_PURPLE                45
#define B_DGREEN                46
#define B_WHITE                 47
#define B_RESTORE               49


#define VGA_COLOR_BLACK         0
#define VGA_COLOR_BLUE          1
#define VGA_COLOR_GREEN         2
#define VGA_COLOR_CYAN          3
#define VGA_COLOR_RED           4
#define VGA_COLOR_MAGENTA       5
#define VGA_COLOR_BROWN         6
#define VGA_COLOR_LIGHT_GREY    7
#define VGA_COLOR_DARK_GREY     8
#define VGA_COLOR_LIGHT_BLUE    9
#define VGA_COLOR_LIGHT_GREEN   10
#define VGA_COLOR_LIGHT_CYAN    11
#define VGA_COLOR_LIGHT_RED     12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_LIGHT_BROWN   14
#define VGA_COLOR_WHITE         15

#define VGA_HIGHT_LIGHT   (1 << 7)

#define VGA_TWINKLE       (1 << 3)

#define VGA_COLOR_LIVE    (( VGA_TWINKLE | (VGA_COLOR_GREEN )) << 8)
#define VGA_COLOR_WARN    (( VGA_TWINKLE | (VGA_COLOR_GREEN | VGA_COLOR_RED )) << 8)

#define TTY_WIDTH 80
#define TTY_HEIGHT 25

#define v2tc(c)                       VT100_COLOR[c]

void tty_set_theme(uint8_t fg, uint8_t bg);

void tty_set_color(uint16_t color);

void tty_put_char(char c);
void tty_put_str(char *str);
void play_cxk_gif();
void _tty_init(void *vga_addr);
void clear_screen();

void tty_clear_line(uint32_t line);
void tty_set_cursor(uint32_t x, uint32_t y);
void tty_scroll_up();

void tty_put_str_at_line(char *str, uint32_t line, uint32_t color);

int tty_flush_buffer(char *buffer, int pos, size_t limit, size_t buffer_size);

void tty_sync_cursor();

#endif
