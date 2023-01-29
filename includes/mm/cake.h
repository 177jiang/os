#ifndef __jyos_cake_h_
#define __jyos_cake_h_

#include <datastructs/jlist.h>

#define PNAME_MAX_LEN       32
#define END_FREE_PIECE      (-1)
#define PILE_CACHELINE      1
#define CACHE_LINE_SIZE     128


struct cake_pile{

    struct list_header  piles;
    struct list_header  full;
    struct list_header  free;
    struct list_header  partial;

    unsigned int        piece_size;
    unsigned int        cakes_count;
    unsigned int        piece_alloced;

    unsigned int        offset;
    unsigned int        pg_per_cake;
    unsigned int        piece_per_cake;

    char name[PNAME_MAX_LEN];

};

typedef unsigned int cpiece_index_t;
struct _cake{

    struct list_header  cakes;
    char                *first_piece;
    unsigned int        next_piece;
    unsigned int        used_piece;

    char                free_pieces[0];

};


struct cake_pile * cake_pile_create(
    char *name,
    unsigned int piece_size,
    unsigned int pg_per_cake,
    int options
  );

void* cake_piece_grub(struct cake_pile *pile);

int cake_piece_release(struct cake_pile *pile, void *vaddr);

void cake_init();

void cake_stats();

#endif

