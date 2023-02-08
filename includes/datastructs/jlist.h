#ifndef __jyos_jlist_h_
#define __jyos_jlist_h_

#include <constant.h>

struct list_header {
    struct list_header* prev;
    struct list_header* next;
};

static inline void
__list_add( struct list_header* elem,
            struct list_header* prev,
            struct list_header* next)
{
    next->prev = elem;
    elem->next = next;
    elem->prev = prev;
    prev->next = elem;
}

static inline void
list_init_head(struct list_header* head) {
    head->next = head;
    head->prev = head;
}

static inline void
list_append(struct list_header* head, struct list_header* elem)
{
    __list_add(elem, head, head->next);
}

static inline void
list_prepend(struct list_header* head, struct list_header* elem)
{
    __list_add(elem, head->prev, head);
}

static inline void
list_delete(struct list_header* elem) {
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;
    
    // make elem orphaned
    elem->prev = elem;
    elem->next = elem;
}

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

static inline int list_empty(struct list_header *head){

    return  head->next == head && head->prev == head;

}
/**
 * list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define list_for_each(pos, n, head, member)				    \
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					            \
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))


struct hash_list_node{

    struct hash_list_node *next;
    struct hash_list_node **pprev;
    //pprev : addr of prev next's addr
};

static inline void hash_list_del(struct hash_list_node *node){

    if(!node->pprev)return;

    *node->pprev      =  node->next;
    node->next->pprev =  node->pprev;
    node->next        =  0;
    node->pprev       =  0;
}

static inline void hash_list_add(struct hash_list_node **head,
                                 struct hash_list_node  *node){
    node->next = *head;
    if(*head){
        (*head)->pprev = &node->next;
    }
    // node->pprev =  &(*head)->next = head;
    node->pprev =  head;
    *head       =  node;
}




#endif
