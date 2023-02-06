
#include <stddef.h>
#include <stdint.h>
#include <libc/stdlib.h>

char base_char[] = "0123456789abcdefghijklmnopqrstuvwxyz";

char * __uitoa_internal
  (uint32_t value, char *str, uint32_t base, uint32_t *size){
    
    if(!base)return NULL;
    
    uint32_t ptr = 0;
    
    do{
      str[ptr++] = base_char[value % base];
      value /= base;
    }while(value);
    
    uint32_t i = 0, j = ptr - 1;
    while(i < j){
      char c = str[i];
      str[i] = str[j];
      str[j] = c;
      ++i,--j;
    }

    str[ptr] = '\0';
    if(size) *size = ptr;
    return str;
}

char *__itoa_internal
  (int32_t value, char *str, uint32_t base, uint32_t *size){

  if(value < 0 && base == 10){
    str[0] = '-';
    uint32_t v = (uint32_t)(-value);
    char * res = __uitoa_internal(v, str + 1, base, size);
    ++*size;
    return res;
  }
  return __uitoa_internal(value, str, base, size);
}

char *itoa(int32_t value, char *str, uint32_t base){
  return __itoa_internal(value, str, base, NULL);
}
