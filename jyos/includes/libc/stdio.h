#ifndef __jyos_stdio_h_
#define __jyos_stdio_h_

// #include <stdarg.h>
// void printf(char *fmt, ... );
// void sprintf(char *buffer, char *fmt, ... );
#include <stdarg.h>
#include <stddef.h>

void _putchar(char character);

int printf_error(const char * format, ...);

// #define printf printf_
int printf_(const char* format, ...);


// #define sprintf sprintf_
int sprintf_(char* buffer, const char* format, ...);


#define snprintf  snprintf_
#define vsnprintf vsnprintf_
int  snprintf_(char* buffer, size_t count, const char* format, ...);
int vsnprintf_(char* buffer, size_t count, const char* format, va_list va);


#define vprintf vprintf_
int vprintf_(const char* format, va_list va);


int fctprintf(void (*out)(char character, void* arg), void* arg, const char* format, ...);

#endif
