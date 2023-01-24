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
