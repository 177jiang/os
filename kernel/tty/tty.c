#include <tty/tty.h>
#include <libc/string.h>
#include <stdint.h>
#include <types.h>
#include <libc/stdio.h>
#include <hal/io.h>

#define JYOS_TEST_CXK

#ifdef  JYOS_TEST_CXK
#include <tty/cxk.h>
  #define CXK_PAGE 694
#else
  #include <tty/cxk2.h>
  #define CXK_PAGE 1
#endif


uint16_t VT100_COLOR [64] = {

    [F_BLACK ] = VGA_COLOR_BLACK        ,
    [F_RED   ] = VGA_COLOR_RED          ,
    [F_GREEN ] = VGA_COLOR_LIGHT_GREEN  ,
    [F_YELLOW] = VGA_COLOR_CYAN         ,
    [F_BLUE  ] = VGA_COLOR_BLUE         ,
    [F_PURPLE] = VGA_COLOR_MAGENTA      ,
    [F_DGREEN] = VGA_COLOR_GREEN        ,
    [F_WHITE ] = VGA_COLOR_WHITE        ,


    [F_RESTORE ] = VGA_COLOR_BLUE       ,

    [B_BLACK ] = VGA_COLOR_BLACK        ,
    [B_RED   ] = VGA_COLOR_RED          ,
    [B_GREEN ] = VGA_COLOR_LIGHT_GREEN  ,
    [B_YELLOW] = VGA_COLOR_CYAN         ,
    [B_BLUE  ] = VGA_COLOR_BLUE         ,
    [B_PURPLE] = VGA_COLOR_MAGENTA      ,
    [B_DGREEN] = VGA_COLOR_GREEN        ,
    [B_WHITE ] = VGA_COLOR_WHITE        ,

    [B_RESTORE] = VGA_COLOR_BLACK       ,

};


uint16_t *vga_buffer = (uint16_t *)0xB8000;

uint32_t TTY_COLUMN = 0;
uint32_t TTY_ROW = 0;
uint16_t TTY_THEME_COLOR= 0x7500;

void tty_set_color(uint16_t color){
  TTY_THEME_COLOR= color;
}
void tty_set_theme(uint8_t fg, uint8_t bg) {
  TTY_THEME_COLOR= (bg << 4 | fg) << 8;
}

void go_next_line(){
    TTY_COLUMN = 0;
    if(++TTY_ROW == TTY_HEIGHT){

    }
}

void tty_put_char(char c) {
  switch (c){
    case '\t':
      TTY_COLUMN += 4;
      break;
    case '\n':
      ++TTY_ROW;
      TTY_COLUMN = 0;
      break;
    case '\r':
      TTY_COLUMN = 0;
    default:
      *(vga_buffer + TTY_COLUMN + TTY_ROW * TTY_WIDTH) =
        (TTY_THEME_COLOR| c);
      ++TTY_COLUMN;
      break;
  }
  if(TTY_COLUMN >= TTY_WIDTH){
    TTY_COLUMN = 0;
    ++TTY_ROW;
  }
  if(TTY_ROW >= TTY_HEIGHT){
    tty_scroll_up();
  }
}

void tty_put_data(uint16_t i, uint16_t j, uint16_t data) {
  *(vga_buffer + j + i * TTY_WIDTH) = data;
}

void tty_put_str(char *str) {
  while(*str) tty_put_char(*str++);
}

void tty_put_str_at_line(char *str, uint32_t line, uint32_t color){

  tty_clear_line(line);
  tty_set_theme(color, VGA_COLOR_BLACK);
  uint32_t x  = TTY_ROW, y = TTY_COLUMN;
  TTY_ROW  = line, TTY_COLUMN  = 0;

  while(*str) tty_put_char(*str++);

  TTY_ROW = x, TTY_COLUMN = y;
  tty_set_theme(VGA_COLOR_BLUE, VGA_COLOR_BLACK);

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

  clear_screen();

  io_outb(0x3D4, 0x0A);
  io_outb(0x3D5, (io_inb(0x3D5) & 0xC0) | 13);

  io_outb(0x3D4, 0x0B);
  io_outb(0x3D5, (io_inb(0x3D5) & 0xE0) | 15);

}

void tty_scroll_up(){

  size_t last_line = TTY_WIDTH * (TTY_HEIGHT - 1);
  memcpy(vga_buffer, vga_buffer + TTY_WIDTH, last_line * 2);
  for(size_t i=0; i<TTY_WIDTH; ++i){
    *(vga_buffer + i + last_line) = TTY_THEME_COLOR;
  }
  TTY_ROW = TTY_ROW ? TTY_HEIGHT-1 : 0;
}

void tty_set_cursor(uint32_t x, uint32_t y){
    if (x >= TTY_WIDTH || y >= TTY_HEIGHT) {
        x = y = 0;
    }
    uint32_t pos = y * TTY_WIDTH + x;
    io_outb(0x3D4, 14);
    io_outb(0x3D5, pos / 256);
    io_outb(0x3D4, 15);
    io_outb(0x3D5, pos % 256);
}

void tty_sync_cursor(){
  tty_set_cursor(TTY_COLUMN, TTY_ROW);
}

void tty_clear_line(uint32_t line){

  asm volatile (
      "rep stosw"
      :
      :"D"(vga_buffer + line * TTY_WIDTH),
       "c"(TTY_WIDTH),
       "a"(TTY_THEME_COLOR)
       :"memory"
  );

}

#define tty_sizeof_char  (sizeof(vga_buffer))

void clear_screen(){

  asm volatile (
      "rep stosw"
      :
      :"D"(vga_buffer),
       "c"(TTY_HEIGHT * TTY_WIDTH),
       "a"(TTY_THEME_COLOR)
       :"memory"
  );

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
    for (int S = 0; S < 10000000/2; ++S) {
      int a;
      a += S - p * 1 / 21;
      a += a - 65535;
      a -= 12345;
      a += a % S + 3;
      a %= 123;
    }
  }
}

static uint16_t inline get_bc(uint16_t c){
  return (c >> 12) & 0xF;
}

static uint16_t inline get_fc(uint16_t c){
  return (c >> 8) & 0xF;
}

static struct {

  uint16_t buf[4096];
  uint16_t pos;

  uint16_t cmd[16];
  uint16_t cpos;

}tty_buf;

static inline __movw(uint32_t s, uint32_t d, uint32_t c){

    asm volatile (
            "movl %1, %%edi\n"
            "movl %2, %%esi\n"
            "rep movsw\n"

            :
            :"c"(c),
             "r"(d),
             "r"(s)
            :"memory", "%edi", "%esi"
    );

}

static inline void tty_put_buf(int *x, int *y){

    if(!tty_buf.pos)return ;

    __movw(
        tty_buf.buf,
        vga_buffer + (*x) + (*y) * TTY_WIDTH,
        tty_buf.pos
    );

    int t = (*x + (tty_buf.pos) % TTY_WIDTH) > TTY_WIDTH;
    *y    = *y + (tty_buf.pos / TTY_WIDTH) + t;
    *x    = ((*x + tty_buf.pos) % TTY_WIDTH);

    tty_buf.pos = 0;
}

#define ERROR_CMD                                    \
    __movw(                                          \
        tty_buf.cmd,                                 \
        tty_buf.buf + tty_buf.pos,                   \
        tty_buf.cpos                                 \
    );                                               \
    tty_buf.pos += tty_buf.cpos;                     \
    tty_buf.cpos = 0;                                \
    _state       = TEXT;

#define THEME(fg, bg)     ((bg << 4 | fg) << 8)

int  tty_flush_buffer(char *buffer, int pos,
                       size_t limit, size_t buffer_size){

    clear_screen();

    uint32_t _format_Value = 0;

    uint16_t Fc = get_fc(TTY_THEME_COLOR),
             Bc = get_bc(TTY_THEME_COLOR);

    int x = 0, y = 0, flag = 0;

    enum { TEXT, ESCAPE, FORMAT, } _state = TEXT;

    tty_buf.cpos = tty_buf.pos = 0;

    while(1){

      if(pos == limit) break;
      int  p = (pos % buffer_size);
      char c = buffer[p];

      switch (_state) {
          case TEXT:
          {
              switch (c) {
                case '\033':
                  tty_put_buf(&x, &y);
                  tty_buf.cmd[tty_buf.cpos++] = (THEME(Fc, Bc) | c);
                  _state = ESCAPE;
                  break;
                case '\t':
                  x += 4;
                  break;
                case '\n':
                  tty_put_buf(&x, &y);
                  ++y;
                  x = 0;
                  break;
                case '\r':
                  x = 0;
                  break;
                default:
                  tty_buf.buf[tty_buf.pos++] = (THEME(Fc, Bc) | c);
                  break;
              }

              if(tty_buf.pos == TTY_WIDTH-1){
                tty_put_buf(&x, &y);
              }

              if(x >= TTY_WIDTH){
                x = 0, ++y;
              }
              if(y >= TTY_HEIGHT){
                --y;
                tty_set_cursor(x, y);
                return pos;
              }
              break;
          }

          case ESCAPE:
          {
              tty_buf.cmd[tty_buf.cpos++] = (THEME(Fc, Bc) | c);
              _format_Value = 0;
              flag          = 0;
              if(c == '['){
                _state = FORMAT;
              }else{
                ERROR_CMD;
              }
              break;
          }

          case FORMAT:
          {
              tty_buf.cmd[tty_buf.cpos++] = c;

              if(is_number(c)){
                  if(++flag == 3){
                      ERROR_CMD;
                  }else{
                    _format_Value = _format_Value * 10 + (c - '0');
                  }
              }else if(c == ';' || c == 'm'){

                  if(_format_Value / 10 == 3){
                      Fc = v2tc(_format_Value);
                  }else if(_format_Value / 10  == 4){
                      Bc = v2tc(_format_Value);
                  }

                  if(c == ';'){
                      _format_Value = 0;
                      flag          = 0;
                      _state        = FORMAT;
                  }else{
                      tty_buf.cpos = 0;
                      _state       = TEXT;
                  }
              }else{
                  ERROR_CMD;
                  //Todo A B C D
              }
              break;
          }

      }

    ++pos;
  }

  tty_put_buf(&x, &y);
  tty_set_cursor(x, y);
  return pos;
}

    //   if(!state && c == '\x033'){
    //       state = 1;
    //   }else if(state == 1 && c == '['){
    //       state = 2;
    //   }else if(state > 1){
    //       if(is_number(c)){
    //           color[state - 2] = v2tc((c - '0') + (color[state - 2] * 10));
    //       }else if(state == 2 && c == ';'){
    //           state = 3;
    //       }else{
    //           if(color[0] == F_RESTORE &&
    //              color[1] == B_RESTORE) {
    //               t_theme = TTY_THEME_COLOR;
    //           } else {
    //               t_theme = ((color[1] << 4) | color[0]) << 8;
    //           }
    //           state = 0;
    //       }
    //   }else{
    //       state = 0;
    //       switch (c) {
    //         case '\t':
    //           x += 4;
    //           break;
    //         case '\n':
    //           ++y;
    //           x = 0;
    //           break;
    //         case '\r':
    //           x = 0;
    //           break;
    //         default:
    //           *(vga_buffer + x + y * TTY_WIDTH) = (t_theme| c);
    //           ++x;
    //           break;
    //       }
    //
    //       if(x >= TTY_WIDTH){
    //         x = 0, ++y;
    //       }
    //       if(y >= TTY_HEIGHT){
    //         --y;
    //         break;
    //       }
    //   }
    //
    //   ++pos;
    //
    // }
    //
    // tty_set_cursor(x, y);











