#include <libc/string.h>
#include <fs/fs.h>
#include <mm/cake.h>
#include <mm/page.h>
#include <mm/valloc.h>
#include <spike.h>


#define PATH_DELIM                  '/'
#define DNODE_HASHTABLE_BITS        10
#define DNODE_HASHTABLE_SIZE        (1 << DNODE_HASHTABLE_BITS)
#define DNODE_HASK_MASK             (DNODE_HASHTABLE_SIZE - 1)
#define DNODE_HASHBITS              (32 - DNODE_HASHTABLE_BITS)


static struct cake_pile             *dnode_pile;
static struct cake_pile             *inode_pile;
static struct cake_pile             *file_pile;
static struct cake_pile             *superblock_pile;

static struct v_superblock          *root_sb;
static struct hash_bucket           *dnode_cache;

static int fs_id = 0;

void vfs_init(){

    dnode_pile      =
        cake_pile_create("dnode_cache", sizeof(struct v_dnode), 1, 0);
    inode_pile      =
        cake_pile_create("inode_cache", sizeof(struct v_inode), 1, 0);
    file_pile       =
        cake_pile_create("file_cache", sizeof(struct v_file), 1, 0);
    superblock_pile =
        cake_pile_create("sb_cache", sizeof(struct v_superblock), 1, 0);
    dnode_cache     =
        valloc(DNODE_HASHTABLE_SIZE * sizeof(struct hash_bucket));

    struct filesystem *root_fs = fsm_get("rootfs");
    root_sb = vfs_sb_alloc();
    root_fs->mount(root_sb, NULL);

}

inline struct hash_bucket *__dcache_get_bucket(
        struct v_dnode *parent, unsigned int hash){

    hash += (uint32_t)parent;
    hash = hash ^ (hash >> DNODE_HASHBITS);
    return dnode_cache + (hash & DNODE_HASK_MASK);
}

struct v_dnode *vfs_dcache_lookup(struct v_dnode *parent,
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
             struct v_dnode **dentry,
             int walk_options){

    int error = 0;
    int i = 0, j = 0;
    if(*path == PATH_DELIM){
        if((walk_options & VFS_WALK_FSRELATIVE)){
            start = start->super_block->root;
        }else{
            start = root_sb->root;
        }
        ++i;
    }
    struct v_dnode *dnode;
    struct v_dnode *current_level = start;
    char tname[VFS_NAME_MAXLEN];
    struct hash_str hs = HASH_STR(tname, 0);
    char cur  = path[i++];
    char next = cur;

    while(cur){

        cur  = next;
        next = path[i++];
        if(cur != PATH_DELIM){
            if(j >= VFS_NAME_MAXLEN - 1){
                return VFS_ETOOLONG;
            }
            if(!VFS_VALID_CHAR(cur)){
                return VFS_EINVLD;
            }
            tname[j++] = cur;

            if(next) continue;
        }

        if(next == PATH_DELIM)continue;

        tname[j] = 0;
        hash_str_rehash(&hs, HSTR_FULL_HASH);
        dnode = vfs_dcache_lookup(current_level, &hs);

        if(!dnode){

            dnode            =  vfs_dnode_alloc();
            dnode->name      =  HASH_STR(valloc(VFS_NAME_MAXLEN), j);
            dnode->name.hash =  hs.hash;
            strcpy(dnode->name.value, hs.value);

            error = current_level->inode->ops.dir_lookup(
                        current_level->inode, dnode);

            int create = (walk_options  & VFS_WALK_MKPARENT) ||
                         ((walk_options & VFS_WALK_MKDIR) && !next);

            if((error == VFS_ENOTFOUND) && create){

                if(!current_level->inode->ops.mkdir){
                    error = VFS_ENOOPS;
                }else{
                    error = current_level->inode->ops.mkdir(
                            current_level->inode, dnode);
                }
            }

            if(error) goto __error;

            vfs_dcache_add(current_level, dnode);
            dnode->parent = current_level;
            list_append(&current_level->children, &dnode->siblings);
        }

        j             =  0;
        current_level =  dnode;
    };

    *dentry = current_level;
    return error;

__error:
    vfree(dnode->name.value);
    vfs_dnode_free(dnode);
    return error;
}

int vfs_open(struct v_dnode *dnode, struct v_file **file){

    if(!dnode->inode || !dnode->inode->ops.open){
        return VFS_ENOOPS;
    }

    struct v_file *f = cake_piece_grub(file_pile);
    memset(f, 0, sizeof(*f));

    int error = dnode->inode->ops.open(dnode->inode, f);

    if(error){
        cake_piece_release(file_pile, f);
    }else{
        *file = f;
    }
    return error;
}

int vfs_close(struct v_file *file){

    if(!file || !file->ops.close){
        return VFS_ENOOPS;
    }

    int error = file->ops.close(file);

    if(!error){
        cake_piece_release(file_pile, file);
    }
    return error;
}

int vfs_fsync(struct v_file *file){

    if(!file || !file->ops.sync){
        return VFS_ENOOPS;
    }
    int error = file->ops.sync(file);
    if(!error || file->inode->ops.sync){
        error = file->inode->ops.sync(file->inode);
    }
    return error;
}

int vfs_mount(const char *fs_name,
              bdev_t device,
              struct v_dnode *mnt_point){

    struct filesystem *fs = fsm_get(fs_name);

    if(!fs)return VFS_ENOFS;

    struct v_superblock *sb = vfs_sb_alloc();
    sb->dev   = device;
    sb->fs_id = fs_id++;

    int error = 0;
    if(!(error = fs->mount(sb, mnt_point))){
        sb->fs   = fs;
        sb->root = mnt_point;
        sb->root->super_block = sb;
        list_append(&root_sb->sb_list, &sb->sb_list);
    }

    return error;

}

int vfs_unmount(struct v_dnode *mnt_point){

    int error = 0;
    struct v_superblock *sb = mnt_point->super_block;
    if(!sb)return VFS_EBADMNT;

    if(!(error = sb->fs->unmount(sb))){

        struct v_dnode *fs_root = sb->root;
        list_delete(&fs_root->siblings);
        list_delete(&sb->sb_list);
        vfs_sb_free(sb);
    }
    return error;
}

struct v_superblock  *vfs_sb_alloc(){

    struct v_superblock *sb = cake_piece_grub(superblock_pile);
    memset(sb, 0, sizeof(*sb));
    list_init_head(&sb->sb_list);
    return sb;
}

void vfs_sb_free(struct v_superblock *sb){

    cake_piece_release(superblock_pile, sb);
}

struct v_dnode *vfs_dnode_alloc(){

    struct v_dnode *dnode = cake_piece_grub(dnode_pile);
    memset(dnode, 0, sizeof (*dnode));
    list_init_head(&dnode->children);
}

void vfs_dnode_free(struct v_dnode *dnode){

    if(dnode->ops.destruct){
        dnode->ops.destruct(dnode);
    }
    cake_piece_release(dnode_pile, dnode);
}

struct v_inode *vfs_inode_alloc(){

    struct v_inode *inode = cake_piece_grub(inode_pile);
    memset(inode, 0, sizeof(*inode));
    return inode;
}

void vfs_inode_free(struct v_inode *inode){

    cake_piece_release(inode_pile, inode);
}






