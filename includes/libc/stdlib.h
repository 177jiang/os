#ifndef __jyos_stdlib_h_
#define __jyos_stdlib_h_

#include <stdint.h>

char *__uitoa_internal(uint32_t value, char *str, uint32_t base, uint32_t *size);
char *__itoa_internal(int32_t value, char *str, uint32_t base, uint32_t *size);
char *itoa(int32_t value, char *str, uint32_t base);

#endif
