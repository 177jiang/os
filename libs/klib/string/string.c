#include <stddef.h>

#include <stdint.h>
#include <libc/string.h>


void *memcpy(void *dest, const void *src, size_t n){

  if( !dest || !src )return NULL;
  if(dest == src) return dest;

  char *d = (char *)dest;
  const char * s = (const char *)src;

  while(n--)*d++ = *s++;

  return dest;
  
}


void *memmove(void *dest, const void *src, size_t n){

  if( !dest || !src )return NULL;
  if(dest == src) return dest;

  char *d = (char *)dest;
  const char *s = (const char *)src;

  if(d < s){
    while(n--)*d++ = *s++;
  }else{
    d += n, s += n;
    while(n--)*--d = *--s;
  }

  return dest;

}

void *memset(void *addr, char c, size_t n){

  if( !addr ) return NULL;

  char *p = (char *) addr;
  while(n--) *p++ = c;

  return addr;

}

int memcmp(const void *p1, const void *p2, size_t n){

  const char *p = (const char *) p1;
  const char *q = (const char *) p2;

  while(n--){
    int d = *p++ - *q++;
    if(d) return d;
  }

  return 0;

}

size_t strlen(const char *str){

  size_t len = 0;
  while(*(str + len))++len;

  return len;

}

char *strcpy(char *dest, const char *src){

  if( !src || !dest )return NULL;

  size_t n = strlen(src);
  memmove(dest, src, n);
  dest[n] = '\0';

  return dest;
}

char *strncpy(char *dest, const char *src, size_t n){
  char c;
  uint32_t i = 0;
  while((c=src[i]) && i < n)dest[i++] = c;
  while(i <= n)dest[i++] = 0;
  return dest;
}





