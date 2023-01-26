#include <mm/pmm.h>
#include <mm/page.h>
#include <stddef.h>

static struct p_page mem_map[MEM_MEP_MAX_SIZE];

static uint32_t MAX_PAGE;

void pmm_mark_page_free(uint32_t ppn){

  mem_map[ppn].ref_counts = 0;

}

void pmm_mark_pages_free(uint32_t start_pnn, uint32_t n){

  for(uint32_t i=start_pnn; i<start_pnn + n && i<MAX_PAGE ; ++i){

    pmm_mark_page_free(i);

  }

}

void pmm_mark_page_occupied(pid_t owner, uint32_t ppn, p_page_attr attr){

  mem_map[ppn] = (struct p_page ){
    .owner  = owner,
    .ref_counts = 1,
    .attr = attr
  };

}

void
pmm_mark_pages_occupied
  (pid_t owner, uint32_t start_pnn, uint32_t n, p_page_attr attr){

    for(uint32_t i=start_pnn; i<start_pnn + n && i<MAX_PAGE; ++i){

      pmm_mark_page_occupied(owner, i, attr);

    }

}

size_t NEXT_PAGE;

void pmm_init(uint32_t mem_upper_limit){

  MAX_PAGE = (PG_ALIGN(mem_upper_limit) >> 12);
  NEXT_PAGE = START_PAGE;

  // pmm_mark_pages_occupied(0, 0, MEM_MEP_MAX_SIZE, 0);
  for(uint32_t i=0; i<MEM_MEP_MAX_SIZE; ++i){
    mem_map[i] = (struct p_page){
      .owner      = 0,
      .attr       = 0,
      .ref_counts = 1
    };
  }

}

Pysical(void *) pmm_alloc_page(pid_t owner, p_page_attr attr){

  size_t pg = NEXT_PAGE, pg_limit = MAX_PAGE;
  struct p_page * page;
  uint32_t page_found = 0;

  while(pg < pg_limit){
    page = mem_map + pg;
    if(!page->ref_counts){
      page_found = pg;
      pmm_mark_page_occupied(owner, pg++, attr);
      break;
    }
    if(++pg >= pg_limit && NEXT_PAGE != START_PAGE){
      pg_limit = NEXT_PAGE;
      pg = START_PAGE;
      NEXT_PAGE = START_PAGE;
    }
  }

  NEXT_PAGE = pg;

  if(!page_found) { }

  return (void *)(page_found << 12);

}

Pysical (void *) pmm_alloc_pages(pid_t owner, size_t n, p_page_attr attr){

  size_t l = START_PAGE, r = START_PAGE;

  while(r < MAX_PAGE && (r - l) < n){

    ((mem_map + r)->ref_counts) ? (l = ++r) : (++r);

  }

  if(r == MAX_PAGE)return NULL;

  pmm_mark_pages_occupied(owner, l, n, attr);

  return (void *)(l << 12);

}

int pmm_free_page(pid_t owner, void *pg){

  struct p_page *page = mem_map + ((uint32_t )pg >> 12);

  if(page->ref_counts == 0)return 0;
  if( ((uintptr_t)pg >> 12) >= MAX_PAGE)return 0;

  if((page->attr & PPG_LOCKED))return 0;

  --page->ref_counts;
  return 1;

}

int pmm_ref_page(pid_t owner, void *p_addr){

  uint32_t n = ((uint32_t)p_addr >> 12);

  if(n >= MAX_PAGE)return 0;

  if(owner){}

  struct p_page *page = mem_map + n;

  if(page->ref_counts == 0)return 0;

  ++page->ref_counts;

  return 1;
}

struct p_page *pmm_query(uintptr_t p_addr){

  uint32_t n = ((uint32_t)p_addr >> 12);
  if(n >= MAX_PAGE)return 0;
  return (mem_map + n);

}




