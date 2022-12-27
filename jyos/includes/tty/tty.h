#ifndef __jyos_tty_h_
#define __jyos_tty_h_

#include <stdint.h>

#define VGA_COLOR_BLACK 0
#define VGA_COLOR_BLUE 1
#define VGA_COLOR_GREEN 2
#define VGA_COLOR_CYAN 3
#define VGA_COLOR_RED 4
#define VGA_COLOR_MAGENTA 5
#define VGA_COLOR_BROWN 6
#define VGA_COLOR_LIGHT_GREY 7
#define VGA_COLOR_DARK_GREY 8
#define VGA_COLOR_LIGHT_BLUE 9
#define VGA_COLOR_LIGHT_GREEN 10
#define VGA_COLOR_LIGHT_CYAN 11
#define VGA_COLOR_LIGHT_RED 12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_LIGHT_BROWN 14
#define VGA_COLOR_WHITE 15

void tty_set_theme(uint8_t fg, uint8_t bg);
void tty_put_char(char c);
void tty_put_str(char *str);
void play_cxk_gif();
void _tty_init(void *vga_addr);
void clear_screen();

void tty_clear_line(uint32_t line);
void tty_set_xy(uint32_t x, uint32_t y);

void tty_put_str_at_line(char *str, uint32_t line, uint32_t color);

void tty_sync_cursor();

#endif
