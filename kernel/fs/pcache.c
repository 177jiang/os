#include <mm/valloc.h>
#include <mm/page.h>
#include <mm/vmm.h>
#include <spike.h>

#include <libc/string.h>
#include <fs/fs.h>

#define PCACHE_DIRTY  1 

void pcache_init(struct pcache *cache){

    list_init_head(&cache->dirty);
    list_init_head(&cache->pages);
    btrie_init(&cache->tree, PG_SIZE_BITS);
    cache->dirty_n = cache->page_n = 0;
}

struct pcache_pg *pcache_new_page(struct pcache *cache, uint32_t index){

    struct pcache_pg *ppg =  vzalloc(sizeof(struct pcache_pg));
    ppg->pg               =  valloc(PG_SIZE);

    list_append(&cache->pages, &ppg->pg_list);
    btrie_set(&cache->tree, index, ppg);
    return ppg;
}

void pcache_set_dirty(struct pcache *cache, struct pcache_pg *page){

    if(!(page->flags & PCACHE_DIRTY)){
        
        page->flags |= PCACHE_DIRTY;
        ++cache->dirty_n;
        list_append(&cache->dirty, &page->dirty_list);
    }
}

int pcache_get_page(struct pcache *cache,
                    uint32_t index,
                    uint32_t *offset,
                    struct pcache_pg **pg){
    
    struct pcache_pg *ppg = btrie_get(&cache->tree, index);
    int new_page = 0;
    *offset = index & ((1 << cache->tree.truncated) - 1);

    if(!ppg){

        ppg       =  pcache_new_page(cache, index);
        ppg->fpos = index - *offset;
        ++cache->page_n;
        new_page =  1;
    }
    *pg = ppg;
    return new_page;
}

int pcache_write(struct v_file *file,
                 char *buf,
                 uint32_t size){


    uint32_t pg_off, buf_off = 0, fpos = file->f_pos;
    struct pcache *cache = file->inode->pg_cache;
    struct pcache_pg *ppg;

    while(size > buf_off){

        pcache_get_page(cache, fpos, &pg_off, &ppg);
        uint32_t writed_bits = MIN(size-buf_off, PG_SIZE-pg_off);
        memcpy(ppg->pg + pg_off, buf + buf_off, writed_bits);

        pcache_set_dirty(cache, ppg);
        buf_off += writed_bits;
        fpos    += writed_bits;
    }
    return buf_off;
}

int pcache_read(struct v_file *file,
                char *buf,
                uint32_t size){

    uint32_t pg_off, buf_off = 0, fpos = file->f_pos;
    struct pcache *cache = file->inode->pg_cache;
    struct pcache_pg *ppg;
    int error = 0;

    while(size > buf_off){
        
        if(pcache_get_page(cache, fpos, &pg_off, &ppg)){
            
            error = file->ops.read(file, ppg->pg, PG_SIZE, ppg->fpos);
            if(error > 0 && error < PG_SIZE){

                size = buf_off + error;
            }else if(error < 0){
                break;
            }
        }

        uint32_t writed_bits = MIN(size-buf_off, PG_SIZE-pg_off);
        memcpy(buf+buf_off, ppg->pg+pg_off, writed_bits);

        buf_off += writed_bits;
        fpos    += writed_bits;
    }

    return error < 0 ? error : buf_off;
}

void pcache_release(struct pcache *cache){
    

    struct pcache_pg *pos, *n;
    list_for_each(pos, n, &cache->pages, pg_list){
        
        vfree(pos);
    }
    btrie_release(&cache->tree);
}


int pcache_commit(struct v_file *file, struct pcache_pg *page){
    
    if(!page){
        return 0;
    }
    if(!(page->flags & PCACHE_DIRTY)){
        return 0;
    }
    
    int error = file->ops.write(file, page->pg, PG_SIZE, page->fpos);
    if(!error){
        page->flags &= ~PCACHE_DIRTY;
        list_delete(&page->pg_list);
        --file->inode->pg_cache->dirty_n;
    }
    return error;
}

void pcache_commit_all(struct v_file *file){
    
    struct pcache    *cache = file->inode->pg_cache;
    struct pcache_pg *pos, *n;

    list_for_each(pos, n, &cache->dirty, dirty_list){
        pcache_commit(file, pos);
    }
}

void pcache_release_page(struct pcache *cache, struct pcache_pg *page){

    list_delete(&page->pg_list);
    vfree(page->pg);
    vfree(page);
    --cache->page_n;
    
}

void pcache_invalidate(struct v_file *file, struct pcache_pg *page){
    
    pcache_commit(file, page);
    pcache_release_page(&file->inode->pg_cache, page);
}
















