#include <fs/fs.h>
#include <libc/string.h>

#include <datastructs/hstr.h>
#include <datastructs/hashtable.h>

#define HASH_BUCKET_BITS       4
#define HASH_BUCKET_NUM        (1 << HASH_BUCKET_BITS)

DECLARE_HASHTABLE(fs_registry, HASH_BUCKET_NUM);

void fsm_init(){

    hashtable_init(fs_registry);
}

void fsm_register(struct filesystem *fs){

    hash_str_rehash(&fs->fs_name, HSTR_FULL_HASH);
    hashtable_hash_in(fs_registry, &fs->fs_list, fs->fs_name.hash);
}

struct filesystem *fsm_get(const char *name){

    struct hash_str hstr = HASH_STR(name, 0);
    hash_str_rehash(&hstr, HSTR_FULL_HASH);

    struct filesystem *pos, *n;
    hashtable_foreach(fs_registry, hstr.hash, pos, n, fs_list){
        if(pos->fs_name.hash == hstr.hash){
            return pos;
        }
    }
    return 0;
}



