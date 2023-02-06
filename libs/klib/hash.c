#include  <libc/hash.h>

uint32_t strhash_32(unsigned char *str, uint32_t truncate_to){

    uint32_t hash = 5318;
    int c;
    while(c=*str++){
        hash = ((hash << 5) + hash) + c;
    }
    return hash >> (HASH_SIZE_BITS - truncate_to);
}
