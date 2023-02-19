#ifndef __jyos_hstr_h_
#define __jyos_hstr_h_

#include <libc/hash.h>

#define HSTR_FULL_HASH     32

struct hash_str{

    unsigned int hash;
    unsigned int len;
    char         *value;
};

#define HASH_STR(str, length) (struct hash_str){.len=(length), .value=(str)}

#define HASH_SEQ(h1, h2) ((h1)->hash == (h2)->hash)


inline void hash_str_rehash(struct hash_str *hs, unsigned int truncate_to){

    hs->hash = strhash_32(hs->value, truncate_to);
}

#endif
