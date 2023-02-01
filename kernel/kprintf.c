#include <stdbool.h>
#include <stdint.h>

#include <libc/stdio.h>
#include <tty/tty.h>

#include <jconsole.h>

static inline void _out_buffer(char character, void* buffer, size_t idx, size_t maxlen) {
  if (idx < maxlen) {
    ((char*)buffer)[idx] = character;
  }
}


static char buffer[1024];

int kprintf_(const char* format, ...) {

  va_list va;

  va_start(va, format);

  const int ret = _vsnprintf(_out_buffer, buffer, (size_t)-1, format, va);

  va_end(va);

  console_write_str(buffer, ret);

  return ret;

}

int kprintf_error(const char* format, ...){

    char fmt[1024];

    sprintf_(
        fmt,
        "\033[31;0m[%s]: %s\033[39;49m",
        "EROR",
        format
    );

    va_list va;

    va_start(va, format);

    const int ret = _vsnprintf(_out_buffer, buffer, (size_t)-1, fmt, va);

    va_end(va);

    console_write_str(buffer, ret);

    return ret;

}

int kprintf_live(const char* format, ...){

    char fmt[1024];

    sprintf_(
        fmt,
        "\033[32;0m[%s]: %s\033[39;49m",
        "NOTICE",
        format
    );

    va_list va;

    va_start(va, format);

    const int ret = _vsnprintf(_out_buffer, buffer, (size_t)-1, fmt, va);

    va_end(va);

    console_write_str(buffer, ret);

    return ret;

}

int kprintf_warn(const char* format, ...){

    char fmt[1024];

    sprintf_(
        fmt,
        "\033[35;0m[%s]: %s\033[39;49m",
        "WARN",
        format
    );

    va_list va;

    va_start(va, format);

    const int ret = _vsnprintf(_out_buffer, buffer, (size_t)-1, fmt, va);

    va_end(va);

    console_write_str(buffer, ret);

    return ret;

}

void kprintf_hex(const void* buffer, unsigned int size) {
    unsigned char* data = (unsigned char*)buffer;
    char buf[16];
    char ch_cache[16];
    unsigned int ptr = 0;
    int i, len;

    ch_cache[0] = '|';
    ch_cache[1] = ' ';
    while (size) {
        len = snprintf_(buf, 64, " %.4p: ", ptr);
        console_write_str(buf, len);
        for (i = 0; i < 8 && size; i++, size--, ptr++) {
            unsigned char c = *(data + ptr) & 0xff;
            ch_cache[2 + i] = (32 <= c && c < 127) ? c : '.';
            len = snprintf_(buf, 64, "%.2x  ", c);
            console_write_str(buf, len);
        }
        ch_cache[2 + i] = '\0';
        console_write_str(ch_cache, i+2);
        console_write_char('\n');
    }
}




