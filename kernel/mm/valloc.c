
#include <mm/valloc.h>
#include <stddef.h>
#include <mm/cake.h>

#define MAX_CLASS  6

static char piles_name[MAX_CLASS][PNAME_MAX_LEN] = {

    "VALLOC_16",  "VALLOC_32",  "VALLOC_64",

    "VALLOC_128", "VALLOC_256", "VALLOC_512"

};

static char piles_name_dma[MAX_CLASS][PNAME_MAX_LEN] = {

    "VALLOC_DMA_128", "VALLOC_DMA_512", "VALLOC_DMA_512",

    "VALLOC_DMA_1K",  "VALLOC_DMA_2K",  "VALLOC_DMA_4K"

};

static struct cake_piles *piles[MAX_CLASS];

static struct cake_piles *piles_dma[MAX_CLASS];


void valloc_init(){

  for(size_t i=0; i<MAX_CLASS; ++i){

      int size = 1 << (i + 4);
      piles[i] = cake_pile_create(
          piles_name[i],
          size,
          1,
          0
      );

  }
  for(size_t i=0; i<MAX_CLASS; ++i){

    int size = 1 << (i + 7);
    piles_dma[i] = cake_pile_create(
        piles_name_dma[i],
        size,
        (size>1024)?(8):(1),
        PILE_CACHELINE
    );

  }

}

void *__valloc(unsigned int size, struct cake_pile **piles){

  struct cake_pile *pile = 0;
  for(size_t i=0; i<MAX_CLASS; ++i){

    if(piles[i]->piece_size >= size){
        pile = piles[i];
        break;
    }

  }

  if(pile == 0)return 0;

  return cake_piece_grub(pile);
}

void __vfree(void *vaddr, struct cake_pile **piles){

    for(size_t i=0; i<MAX_CLASS; ++i){
        if(cake_piece_release(piles[i], vaddr)){
          break;
        }
    }

    return ;

}


void *valloc(unsigned int size){
    return __valloc(size, piles);
}


void *valloc_dma(unsigned int size){
    return __valloc(size, piles_dma);
}

void vfree(void *vaddr){
    return __vfree(vaddr, piles);
}

void vfree_dma(void *vaddr){
    return __vfree(vaddr, piles_dma);
}








