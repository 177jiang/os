#ifndef __jyos_hashtable_h_
#define __jyos_hashtable_h_

#include <libc/hash.h>
#include <datastructs/jlist.h>


struct hash_bucket{

    struct hash_list_node *head;
};

#define table_size(table) ( sizeof(table) / sizeof(table[0]) )

#define __hash_key(table, hash)  ( (hash) % table_size(table) )

#define DECLARE_HASHTABLE(name, bucket_num) \
    struct   hash_bucket name[bucket_num];

#define hashtable_bucket_foreach(bucket, pos, n, member)                 \
    for(pos=list_entry((bucket)->head, typeof(*pos), member);           \
        pos && ({                                                        \
            n = list_entry(pos->member.next, typeof(*pos), member);      \
            1;                                                           \
          });                                                            \
        pos = n)


#define hashtable_foreach(table, hash, pos, n, member)              \
    hashtable_bucket_foreach(table + __hash_key(table, hash), pos, n, member)


#define hashtable_init(table){                                     \
    for(int i=0; i<table_size(table); ++i){                        \
        table[i].head = 0;                                         \
    }                                                              \
}


#define hashtable_hash_in(table, list_node, hash)                  \
    hash_list_add(&(table+__hash_key(table, hash))->head, list_node)

#endif
