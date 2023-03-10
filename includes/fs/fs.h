#ifndef __jyos_fs_h_
#define __jyos_fs_h_


#include <hal/ahci/hba.h>
#include <stddef.h>
#include <block.h>
#include <types.h>
#include <status.h>

#include <datastructs/hashtable.h>
#include <datastructs/hstr.h>
#include <datastructs/jlist.h>
#include <datastructs/btrie.h>

#define IN(x)  (x)
#define OUT(x) (x)

#define     VFS_NAME_MAXLEN             128
#define     VFS_FD_MAX                  32

#define     VFS_INODE_TYPE_DIR          0x1 
#define     VFS_INODE_TYPE_FILE         0x2 
#define     VFS_INODE_TYPE_DEVICE       0x4 
#define     VFS_INODE_TYPE_SYMLINK      0x8 


#define     VFS_ENOFS                   -2
#define     VFS_EBADMNT                 -3

#define     VFS_EENDDIR                 -5

#define     VFS_EINVALD                  -8
#define     VFS_EEOF                    -9

#define     VFS_WALK_MKPARENT           1
#define     VFS_WALK_FSRELATIVE         2
#define     VFS_WALK_MKDIR              3
#define     VFS_WALK_PARENT             4
#define     VFS_WALK_NOFOLLOW           8

#define     VFS_IOBUF_FDIRTY            0x1

#define FSTYPE_ROFS                     0x1

#define     VFS_VALID_CHAR(c)           \
    (is_alpha(c) || is_number(c)  ||    \
    ((c)=='.')  || ((c)=='-')    || ((c)=='_') )

#define  DI_OPS(dnode)   (dnode->inode->ops)

extern struct hash_str vfs_dot;
extern struct hash_str vfs_ddot;

struct pcache;
struct v_dnode;
struct v_superblock;
struct v_file;

struct filesystem{

    struct hash_list_node fs_list;
    struct hash_str       fs_name;
    uint32_t              type;
    int (*mount)(struct v_superblock *vsb, struct v_dnode *mount_point);
    int (*unmount)(struct v_superblock *vsb);
};



struct v_inode{

    uint32_t  itype;
    uint32_t  ctime;
    uint32_t  mtime;
    uint64_t  lb_addr;
    uint32_t  lb_usage;
    uint32_t  open_count;
    uint32_t  link_count;
    uint32_t  fsize;
    void      *data;

    struct   pcache *pg_cache;
    struct {

        int (*create)       (struct v_inode *this);
        int (*open)         (struct v_inode *this, struct v_file *file);
        int (*sync)         (struct v_inode *this);
        int (*mkdir)        (struct v_inode *this, struct v_dnode *dnode);
        int (*rmdir)        (struct v_inode *this);
        int (*unlink)       (struct v_inode *this);
        int (*link)         (struct v_inode *this, struct v_dnode *dnode);
        int (*dir_lookup)   (struct v_inode *this, struct v_dnode *dnode);
        int (*symlink)      (struct v_inode *this, const char *path);
        int (*read_symlink) (struct v_inode *this, const char **path_out);
    }ops;
};

struct v_dnode{

    struct hash_str         name;
    struct v_inode          *inode;
    struct v_dnode          *parent;
    struct hash_list_node   hash_list;
    struct list_header      children; 
    struct list_header      siblings; 
    struct v_superblock     *super_block;
    struct {
        void (*destruct)(struct v_dnode *dnode);
    }ops;
};

struct v_fdtable{

    struct v_fd *fds[VFS_FD_MAX];
};

struct v_file_ops{
    int (*write)    (struct v_file *this, void IN(*data), uint32_t size, uint32_t fpos);
    int (*read)     (struct v_file *this, void OUT(*data), uint32_t size, uint32_t fpos);
    int (*read_dir) (struct v_file *this, int dir_index);
    int (*seek)     (struct v_file *this, size_t offset);
    int (*rename)   (struct v_file *this, char *name);
    int (*close)    (struct v_file *this);
    int (*sync)     (struct v_file *this);
};

struct v_file{

    struct v_inode      *inode;
    struct v_dnode      *dnode;
    struct list_header  *f_list;
    uint32_t            f_pos;
    uint32_t            ref_count;
    void                *data;
    struct v_file_ops   ops;
};

struct v_fd{

    struct v_file *file;
    int            flags;
};

struct v_superblock{

    struct list_header  sb_list;
    int                 fs_id;
    bdev_t              dev;
    struct v_dnode      *root;
    struct filesystem   *fs;
    uint32_t            iobuf_size;

    struct {

        uint32_t (*read_capacity)(struct v_superblock *this);
        uint32_t (*read_usage)(struct v_superblock *this);
    }ops;
};

struct dir_context{

    int index;
    void *cb_data;
    void (*read_complete_callback)(
            struct dir_context *this,
            const char *name,
            int len,
            const int   dtype
        );
};

struct pcache_pg{

    struct list_header  pg_list;
    struct list_header  dirty_list;
    uint32_t            flags;
    uint32_t            fpos;
    char                *pg;
};

struct pcache{

    struct btrie        tree;
    struct list_header  pages;
    struct list_header  dirty;
    uint32_t            dirty_n;
    uint32_t            page_n;
};

void   fsm_init();
void   fsm_register(struct filesystem *fs);
struct filesystem *fsm_get(const char * fs_name);

void   vfs_init();

struct v_dnode *vfs_dcache_lookup(
        struct v_dnode *parent, struct hash_str *hs);

void   vfs_dcache_add(
        struct v_dnode *parent, struct v_dnode *node);

int    vfs_walk(
        struct v_dnode *start,
        const char *path,
        struct v_dnode **dentry,
        struct hash_str *compoent,
        int walk_options);

int vfs_mount(
        const char *target, const char *fs_name, bdev_t dev);
int vfs_unmount(const char *target);

int vfs_mount_at(
        const char *fs_name,
        bdev_t device,
        struct v_dnode *mnt_point);

int vfs_unmount_at(struct v_dnode *mnt_point);

int vfs_mkdir(
        const char *path,
        struct v_dnode **dentry);

int vfs_open(struct v_dnode *dnode, struct v_file **file);

int vfs_close(struct v_file *file);

int vfs_fsync(struct v_file *file);

struct v_superblock *vfs_sb_alloc();
void                 vfs_sb_free(struct v_superblock *sb);

struct v_dnode *vfs_dnode_alloc();
void            vfs_dnode_free(struct v_dnode *dnode);

struct v_inode *vfs_inode_alloc();
void            vfs_inode_free(struct v_inode *inode);


void pcache_init(struct pcache *cache);

struct pcache_pg *pcache_new_page(struct pcache *cache, uint32_t index);

void pcache_set_dirty(struct pcache *cache, struct pcache_pg *page);

int pcache_get_page(struct pcache *cache,
                    uint32_t index,
                    uint32_t *offset,
                    struct pcache_pg **pg);

int pcache_write(struct v_file *file,
                 char *buf,
                 uint32_t size);

int pcache_read(struct v_file *file,
                char *buf,
                uint32_t size);

void pcache_release(struct pcache *cache);

int pcache_commit(struct v_file *file, struct pcache_pg *page);

void pcache_commit_all(struct v_file *file);

void pcache_release_page(struct pcache *cache, struct pcache_pg *page);

void pcache_invalidate(struct v_file *file, struct pcache_pg *page);

#endif
















