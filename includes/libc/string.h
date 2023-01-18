#ifndef __jyos_string_h_
#define __jyos_string_h_

#include <stddef.h>


void *memcpy(void *dest, const void *src, size_t n);

void *memmove(void *dest, const void *src, size_t n);

void *memset(void *addr, char c, size_t n);

int memcmp(const void *p1, const void *p2, size_t n);

size_t strlen(const char *str);

char *strcpy(char *dest, const char *src);

char *strncpy(char *dest, const char *src, size_t n);


#endif
