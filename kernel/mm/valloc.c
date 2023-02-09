
#include <mm/valloc.h>
#include <stddef.h>
#include <mm/cake.h>

#define CLASS_LEN(class)  (sizeof(class) / sizeof(class[0]))

static char piles_name[][PNAME_MAX_LEN] = {


    "VALLOC_8", "VALLOC_16",  "VALLOC_32",  "VALLOC_64", "VALLOC_128",

    "VALLOC_256", "VALLOC_512", "VALLOC_1K", "VALLOC_2K", "VALLOC_4K"

};

static char piles_name_dma[][PNAME_MAX_LEN] = {

    "VALLOC_DMA_128", "VALLOC_DMA_256", "VALLOC_DMA_512",

    "VALLOC_DMA_1K",  "VALLOC_DMA_2K",  "VALLOC_DMA_4K"

};

static struct cake_piles *piles[CLASS_LEN(piles_name)];

static struct cake_piles *piles_dma[CLASS_LEN(piles_name_dma)];


void valloc_init(){

  size_t size = CLASS_LEN(piles_name);
  for(size_t i=0; i<size; ++i){

      int size = 1 << (i + 3);
      piles[i] = cake_pile_create(
          piles_name[i],
          size,
          (size > 1024) ? (4) : (1),
          0
      );

  }

  size = CLASS_LEN(piles_name_dma);
  for(size_t i=0; i<size; ++i){

    int size = 1 << (i + 7);
    piles_dma[i] = cake_pile_create(
        piles_name_dma[i],
        size,
        (size > 1024) ? (4) : (1),
        PILE_CACHELINE
    );

  }

}

void *__valloc(unsigned int size, struct cake_pile **piles, size_t tlen){

  struct cake_pile *pile = 0;
  for(size_t i=0; i<tlen; ++i){

    if(piles[i]->piece_size >= size){
        pile = piles[i];
        break;
    }

  }

  if(pile == 0)return 0;

  return cake_piece_grub(pile);
}

void __vfree(void *vaddr, struct cake_pile **piles, size_t tlen){

    for(size_t i=0; i<tlen; ++i){
        if(cake_piece_release(piles[i], vaddr)){
            break;
        }
    }

    return ;

}


void *valloc(unsigned int size){
    return __valloc(size, piles, CLASS_LEN(piles_name));
}

void *vzalloc(unsigned int size){

    void *res =  __valloc(size, piles, CLASS_LEN(piles_name));
    memset(res, 0, size);
    return res;
}

void *vcalloc(unsigned int size, unsigned int count){

    unsigned int alloc_size;
    if(__builtin_umul_overflow(size, count, &alloc_size)){
        return 0;
    }
    return vzalloc(alloc_size);
}

void *valloc_dma(unsigned int size){
    return __valloc(size, piles_dma, CLASS_LEN(piles_name_dma));
}

void *vzalloc_dma(unsigned int size){

    void *res =  __valloc(size, piles_dma, CLASS_LEN(piles_name_dma));
    memset(res, 0, size);
    return res;
}

void *vcalloc_dma(unsigned int size, unsigned int count ){

    unsigned int alloc_size;
    if(__builtin_umul_overflow(size, count, &alloc_size)){
        return 0;
    }
    return vzalloc_dma(alloc_size);
}


void vfree(void *vaddr){
    return __vfree(vaddr, piles, CLASS_LEN(piles_name));
}

void vfree_dma(void *vaddr){
    return __vfree(vaddr, piles_dma, CLASS_LEN(piles_name_dma));
}








