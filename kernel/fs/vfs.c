#include <libc/string.h>
#include <fs/fs.h>
#include <mm/cake.h>
#include <mm/valloc.h>


#define PATH_DELIM                  '/'
#define DNODE_HASHTABLE_BITS        10
#define DNODE_HASHTABLE_SIZE        (1 << DNODE_HASHTABLE_BITS)
#define DNODE_HASK_MASK             (DNODE_HASHTABLE_SIZE - 1)
#define DNODE_HASHBITS              (32 - DNODE_HASHTABLE_BITS)

#define VFS_ETOOLONG                -1
#define VFS_ENOFS                   -2

static struct cake_pile             *dnode_pile;
static struct cake_pile             *inode_pile;
static struct cake_pile             *superblock_pile;
static struct cake_pile             *name_pile;

static struct v_superblock          *root_sb;
static struct hash_bucket           *dnode_cache;

static int fs_id = 0;

struct v_dnode *__new_dnode();
void __free_dnode(struct v_dnode *dnode);

struct v_superblock *__new_superblock();
void __free_superblock(struct v_superblock *sb);

void vfs_init(){

    dnode_pile      =  cake_pile_create("dnode", sizeof(struct v_dnode), 1, 0);
    inode_pile      =  cake_pile_create("inode", sizeof(struct v_inode), 1, 0);
    name_pile       =  cake_pile_create("dname", VFS_NAME_MAXLEN, 1, 0);
    superblock_pile =
      cake_pile_create("v_superblock", sizeof(struct v_superblock), 1, 0);
    dnode_cache     = valloc(DNODE_HASHTABLE_SIZE * sizeof(struct hash_bucket));

    struct filesystem *root_fs = fsm_get("root_fs");
    root_sb = __new_superblock();
    root_fs->mount(root_sb, NULL);

}

inline struct hash_bucket
  *__dcache_get_bucket(struct v_dnode *parent, unsigned int hash){

    hash += (uint32_t)parent;
    hash = hash ^ (hash >> DNODE_HASHBITS);
    return dnode_cache + (hash & DNODE_HASK_MASK);
}

struct v_dnode *vfs_dnode_lookup(struct v_dnode *parent,
                                 struct hash_str *hstr){

    struct hash_bucket *bucket = __dcache_get_bucket(parent, hstr->hash);

    struct v_dnode *pos, *n;
    hashtable_bucket_foreach(bucket, pos, n, hash_list){

        if(pos->name.hash == hstr->hash){
            return pos;
        }
    }
    return 0;
}

void vfs_dcache_add(struct v_dnode *parent, struct v_dnode *node){

    struct hash_bucket *bucket =
      __dcache_get_bucket(parent, node->name.hash);
    hash_list_add(&bucket->head, &node->hash_list);
}

int vfs_walk(struct v_dnode *start,
             const char *path,
             struct v_dnode **dentry){

    int error = 0;
    int i = 0, j = 0;
    if(*path == PATH_DELIM){
        start = start->super_block->root;
        ++i;
    }
    char tname[VFS_NAME_MAXLEN];
    char cur, next;
    struct hash_str hs = HASH_STR(tname, 0);
    struct v_dnode *dnode;
    struct v_dnode *current_level = start;
    do{

        cur  = path[i++];
        next = path[i++];
        if(cur != PATH_DELIM){
            if(j >= VFS_NAME_MAXLEN - 1){
                error = VFS_ETOOLONG;
                goto error;
            }
            tname[j++] = cur;
            if(next) continue;
        }

        tname[j] = 0;
        hash_str_rehash(&hs, 32);
        dnode = vfs_dnode_lookup(current_level, &hs);

        if(!dnode){

            dnode = __new_dnode();
            dnode->name.hash = hs.hash;
            dnode->name.len  = j;
            strcpy(dnode->name.value, hs.value);

            error = dnode->inode->ops.dir_lookup(
                        current_level->inode, dnode);
            if(error){
                goto error;
            }

            vfs_dcache_add(current_level, dnode);
            dnode->parent = current_level;
            list_append(&current_level->children, &dnode->siblings);
        }

        j = 0;
        current_level = dnode;
    }while(next);

error:
    return error;
}

int vfs_mount(const char *fs_name,
              bdev_t device,
              struct v_dnode *mnt_point){

    struct filesystem *fs = fsm_get(fs_name);
    if(!fs)return 0;

    struct v_superblock *sb = __new_superblock();
    sb->dev   = device;
    sb->fs_id = fs_id++;

    int error = 0;
    if(!(error = fs->mount(sb, mnt_point))){
        sb->fs = fs;
        list_append(&root_sb->sb_list, &sb->sb_list);
    }

    return error;

}

int vfs_unmount(const int fs_id){

    int error = 0;
    struct v_superblock *pos, *n;
    list_for_each(pos, n, &root_sb->sb_list, sb_list){

        if(pos->fs_id != fs_id)continue;
        if(!(error = pos->fs->unmount(pos))){
            list_delete(&pos->sb_list);
            __free_superblock(pos);
        }
        break;
    }
    return error;
}

struct v_superblock  *__new_superblock(){

    struct v_superblock *sb = cake_piece_grub(superblock_pile);
    memset(sb, 0, sizeof(*sb));
    list_init_head(&sb->sb_list);
    sb->root = __new_dnode();
}

void __free_superblock (struct v_superblock *sb){

    __free_dnode(sb->root);
    cake_piece_release(superblock_pile, sb);
}

struct v_dnode *__new_dnode(){

    struct v_dnode *dnode = cake_piece_grub(dnode_pile);
    memset(dnode, 0, sizeof (*dnode));
    dnode->inode      =  cake_piece_grub(inode_pile);
    dnode->name.value =  cake_piece_grub(name_pile);
}

void __free_dnode(struct v_dnode *dnode){

    cake_piece_release(name_pile,  dnode->name.value);
    cake_piece_release(inode_pile, dnode->inode);
    cake_piece_release(dnode_pile, dnode);
}







