#ifndef __jyos_btrie_h_
#define __jyos_btrie_h_

#include <datastructs/jlist.h>
#include <types.h>

#define BTRIE_BITS          4
#define BTRIE_INSERT        1

struct btrie{
  
    struct btrie_node *btrie_root;
    int                truncated;
};

struct btrie_node{
    
    struct list_header  children;
    struct list_header  siblings;
    struct list_header  nodes;
    struct btrie_node  *parent;
    uint32_t            index;
    void                *data;
};

void btrie_init(struct btrie *root, uint32_t trun_bits);
void *btrie_set(struct btrie *root, uint32_t index, void *data);
void *btrie_get(struct btrie *root, uint32_t index);
void *btrie_remove(struct btrie *root, uint32_t index);
void *btrie_release(struct btrie *root);



#endif
