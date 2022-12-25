
#include <jyos/tty/tty.h>
#include <libc/string.h>
#include <stdint.h>

#ifdef  JYOS_TEST_CXK
  #include <jyos/tty/cxk.h>
  #define CXK_PAGE 694
#else
  #include <jyos/tty/cxk2.h>
  #define CXK_PAGE 1
#endif



#define TTY_WIDTH 80
#define TTY_HEIGHT 25

uint16_t *vga_buffer = (uint16_t *)0xB8000;

uint32_t TTY_COLUMN = 0;
uint32_t TTY_ROW = 0;
uint16_t THEME_COLOR = 0x7500;


#define tty_sizeof_char  (sizeof(vga_buffer))
void clear_screen(){
  memset(vga_buffer, 0, tty_sizeof_char);
}

void tty_set_theme(uint8_t fg, uint8_t bg) {
  THEME_COLOR = (bg << 4 | fg) << 8;
}

void go_next_line(){
    TTY_COLUMN = 0;
    if(++TTY_ROW == TTY_HEIGHT)TTY_ROW = 0;
}
void tty_put_char(char c) {
  if(c == '\n'){
    go_next_line();
    return;
  }
  *(vga_buffer + TTY_COLUMN + TTY_ROW * TTY_WIDTH) = (THEME_COLOR | c);
  if (++TTY_COLUMN == TTY_WIDTH) {
    TTY_COLUMN = 0;
    if(++TTY_ROW == TTY_HEIGHT){
      clear_screen();
      TTY_ROW = 0;
    }
  }
}

void tty_put_data(uint16_t i, uint16_t j, uint16_t data) {
  *(vga_buffer + j + i * TTY_WIDTH) = data;
}

void tty_put_str(char *str) {
  while(*str) tty_put_char(*str++);
}

uint16_t random_color(uint16_t p, uint16_t i, uint16_t j) {
  uint16_t color = 0;
  color |= (i + j + p) >> 2;
  color <<= 8;
  color &= 0xFF00;

  return (uint16_t)color;
}

void _tty_init(void *addr){

  vga_buffer = (uint16_t *)addr;

  tty_set_theme(VGA_COLOR_BLUE, VGA_COLOR_BLACK);

}

void tty_set_sx(uint32_t x, uint32_t y){
  TTY_COLUMN = y, TTY_ROW = x;
}

void tty_clear_line(uint32_t line){

  for(int i=0; i<TTY_WIDTH; ++i){
    tty_put_data(line, i, 0);
  }

}


void play_cxk_gif() {
  clear_screen();
  for (uint16_t p = 0; p < CXK_PAGE; ++p) {
    for (uint16_t i = 0; i < TTY_HEIGHT; ++i) {
      for (uint16_t j = 0; j < TTY_WIDTH; ++j) {
        char c = *( SEG_TABLE + cxk_map[p][i][j] );
        uint16_t color = 0;
        if (c != ' ') {
          color = random_color(p, i, j);
        }
        tty_put_data(i, j, color | c);
      }
    }
    for (int S = 0; S < 10000000 * 2; ++S) {
      int a;
      a += S - p * 1 / 21;
      a += a - 65535;
      a -= 12345;
      a += a % S + 3;
      a %= 123;
    }
  }
}
