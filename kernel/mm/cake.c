
#include <mm/cake.h>
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <libc/stdio.h>
#include <spike.h>

struct cake_pile master_pile ;

struct list_header piles = {
    .next = &piles,
    .prev = &piles
};

void *__cake_alloc(unsigned int pg_count){

    uintptr_t paddr = pmm_alloc_pages(KERNEL_PID, pg_count, 0);
    return vmm_vmap(paddr, pg_count * PG_SIZE, PG_PREM_RW);

}

struct _cake * __cake_new(struct cake_pile *pile){

    struct _cake *cake = __cake_alloc(pile->pg_per_cake);

    if(!cake)return NULL;

    cake->first_piece =  ((char*)cake) + pile->offset;
    cake->next_piece  =  0;
    cake->used_piece  =  0;

    cpiece_index_t *pieces_free = &cake->free_pieces;

    for(size_t i=0; i<pile->piece_per_cake-1; ++i){

        pieces_free[i] = i + 1;

    }

    pieces_free[pile->piece_per_cake-1] = END_FREE_PIECE;

    list_append(&pile->free, &cake->cakes);

    return cake;
}

void __cake_pile_init(
        struct cake_pile *pile,
        char *name,
        unsigned int piece_size,
        unsigned int pg_per_cake,
        int options
    ){

    unsigned int align = sizeof(long);

    if(( options & PILE_CACHELINE )){
        align = CACHE_LINE_SIZE;
    }

    piece_size = ROUNDUP(piece_size, align);

    *pile = (struct cake_pile){

        .piece_size     =  piece_size,
        .cakes_count    =  1,
        .pg_per_cake    =  pg_per_cake,
        .piece_per_cake =
            (pg_per_cake * PG_SIZE) / (piece_size + sizeof(cpiece_index_t)),

    };

    unsigned int header_size =
        sizeof(struct _cake) + (sizeof(cpiece_index_t) * (pile->piece_per_cake));

    pile->offset = ROUNDUP(header_size, align);

    list_init_head(&pile->free);
    list_init_head(&pile->full);
    list_init_head(&pile->partial);
    list_append(&piles, &pile->piles);

    strncpy(pile->name, name, PNAME_MAX_LEN);

}

void cake_init(){

    __cake_pile_init(&master_pile, "pinkamina", sizeof(master_pile), 1, 0);

}



struct cake_pile * cake_pile_create(
    char *name,
    unsigned int piece_size,
    unsigned int pg_per_cake,
    int options
  ){

    struct cake_pile *pile = cake_piece_grub(&master_pile);

    __cake_pile_init(pile, name, piece_size, pg_per_cake, options);

    return pile;

}

void* cake_piece_grub(struct cake_pile *pile){

    if(!pile)return 0;

    struct _cake *pos, *n;

    list_for_each(pos, n, &pile->partial, cakes){
        if(pos->next_piece != END_FREE_PIECE){
            goto __found;
        }
    }

    if(list_empty(&pile->free)){
        pos = __cake_new(pile);
    }else{
        pos = list_entry(&pile->free.next, typeof(*pos), cakes);
    }

__found:

    cpiece_index_t found = pos->next_piece;
    pos->next_piece = pos->free_pieces[found];
    ++pos->used_piece;
    ++pile->piece_alloced;

    list_delete(&pos->cakes);

    if(pos->next_piece == END_FREE_PIECE){
        list_append(&pile->full, &pos->cakes);
    }else{
        list_append(&pile->partial, &pos->cakes);
    }

    return (void*)(pos->first_piece + found * pile->piece_size);

}

int cake_piece_release(struct cake_pile *pile, void *vaddr){

    struct list_header *hs[2] = {
        &pile->full,
        &pile->partial,
    };

    cpiece_index_t found = 0;
    struct _cake *pos, *n;
    for(int i=0; i<2; ++i) {

        list_for_each(pos, n, hs[i], cakes){
            if(pos->first_piece > vaddr){
                continue;
            }
            found = ((char*)vaddr - pos->first_piece) / (pile->piece_size);
            if(found < pile->piece_per_cake){
                goto __found;
            }
        }

    }

    return 0;
__found:

    pos->free_pieces[found] =  pos->next_piece;
    pos->next_piece         =  found;
    --pos->used_piece;
    --pile->piece_alloced;

    list_delete(&pos->cakes);

    if(pos->used_piece == 0){
        list_append(&pile->full, &pos->cakes);
    }else{
        list_append(&pile->partial, &pos->cakes);
    }

    return 1;

}


void cake_stats(){

    kprintf_live("<name>   <cake>    <pg/c>   <p/c>   <alloced>\n");

    struct cake_pile *pos, *n;
    list_for_each(pos, n, &piles, piles) {

        kprintf_warn("%s %d %d %d %d\n",
                pos->name,
                pos->cakes_count,
                pos->pg_per_cake,
                pos->piece_per_cake,
                pos->piece_alloced);
    }

}














