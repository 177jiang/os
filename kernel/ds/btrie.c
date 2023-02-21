
#include <datastructs/btrie.h>
#include <mm/valloc.h>
#include <spike.h>
#include <stddef.h>


void btrie_init(struct btrie *root, uint32_t trun_bits){

   root->btrie_root = vzalloc(sizeof(struct btrie_node)); 
   root->truncated  = trun_bits;
   list_init_head(&root->btrie_root->nodes);
}

struct btrie_node *__btrie_traversal(struct btrie *root,
                       uint32_t index,
                       int options){

    index >>= root->truncated;
    uint32_t bit_shift =
      index ? (ROUNDUP(31-__builtin_clz(index), BTRIE_BITS)) : 0;
    uint32_t bit_mask   =
      ((1 << BTRIE_BITS) - 1) << bit_shift;
    struct btrie_node *tree = root;

    while(bit_shift && tree){

        uint32_t i = (index & bit_mask) >> bit_shift;
        struct btrie_node *pos, *n, *subtree = 0;

        list_for_each(pos, n, &tree->children, siblings){
            if(pos->index == i){
                subtree = pos;
                break;
            }
        }

        if(!subtree && (options & BTRIE_INSERT)){

          subtree         =  vzalloc(sizeof(struct btrie_node));
          subtree->index  =  i;
          subtree->parent =  tree;

          list_init_head(&subtree->children);
          list_append(&tree->children, &subtree->siblings);
          list_append(&root->btrie_root->nodes, &subtree->nodes);
        }

        tree = subtree;
        bit_mask >>= BTRIE_BITS;
        bit_shift -= BTRIE_BITS;
    }
    return tree;
}


void __btrie_remove(struct btrie_node *node){
    
    struct btrie_node *parent = node->parent;

    if(!parent)return ;

    list_delete(&node->siblings);
    list_delete(&node->nodes);
    vfree(node);
    if(list_empty(&parent->children)){
        __btrie_remove(parent);
    }
}

void *btrie_remove(struct btrie *root, uint32_t index){
    
    struct btrie_node *node  = __btrie_traversal(root, index, 0);
    if(!node) return 0;

    void *data = node->data;
    __btrie_remove(node);
    return data;
}

void *btrie_set(struct btrie *root, uint32_t index, void *data){

    __btrie_traversal(root, index, BTRIE_INSERT)->data = data;
}
void *btrie_get(struct btrie *root, uint32_t index){
    
    struct btrie_node * node = __btrie_traversal(root, index, 0);
    if(node) return node->data;
    return 0;
}


void *btrie_release(struct btrie *root){
    
    struct btrie_node *pos, *n;
    list_for_each(pos, n, &root->btrie_root->nodes, nodes){
        vfree(pos);
    }
    vfree(root->btrie_root);
}

