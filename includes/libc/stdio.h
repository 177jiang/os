#ifndef __jyos_stdio_h_
#define __jyos_stdio_h_

#include <stdarg.h>
#include <stddef.h>

// output function type
typedef void (*out_fct_type)(char character, void* buffer, size_t idx, size_t maxlen);

void _putchar(char character);


// #define printf printf_
int kprintf_(const char* format, ...);
int kprintf_error(const char * format, ...);
int kprintf_warn(const char * format, ...);
int kprintf_live(const char * format, ...);


// #define sprintf sprintf_
int sprintf_(char* buffer, const char* format, ...);


#define snprintf  snprintf_
#define vsnprintf vsnprintf_
int  snprintf_(char* buffer, size_t count, const char* format, ...);
int vsnprintf_(char* buffer, size_t count, const char* format, va_list va);
int _vsnprintf(out_fct_type out, char* buffer, const size_t maxlen, const char* format, va_list va);


#define vprintf vprintf_
int vprintf_(const char* format, va_list va);


int fctprintf(void (*out)(char character, void* arg), void* arg, const char* format, ...);

#endif
