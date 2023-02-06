#ifndef __jyos_hash_h_
#define __jyos_hash_h_

#include <stdint.h>

#define HASH_SIZE_BITS   32

uint32_t strhash_32(unsigned char *data, uint32_t truncate_to);

inline uint32_t hash_32(uint32_t val, uint32_t truncate_to){

  return
    (val * 0x61C88647u) >> (HASH_SIZE_BITS - truncate_to);
}


#endif
