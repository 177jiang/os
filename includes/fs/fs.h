#ifndef __jyos_fs_h_
#define __jyos_fs_h_


#include <hal/ahci/hba.h>
#include <stddef.h>
#include <block.h>
#include <datastructs/hashtable.h>
#include <datastructs/hstr.h>
#include <datastructs/jlist.h>

#define IN(x)  (x)
#define OUT(x) (x)

#define   VFS_NAME_MAXLEN     128

struct v_dnode;
struct v_superblock;
struct v_file;

struct filesystem{

    struct hash_list_node fs_list;
    struct hash_str       fs_name;

    int (*mount)(struct v_superblock *vsb, struct v_dnode *mount_point);
    int (*unmount)(struct v_superblock *vsb);
};



struct v_inode{

    uint32_t  ityep;
    uint32_t  ctime;
    uint32_t  mtime;
    uint64_t  lb_addr;
    uint32_t  ref_count;
    uint32_t  lb_usage;

    struct {

        int (*open)       (struct v_inode *this, struct v_file *file);
        int (*sync)       (struct v_inode *this);
        int (*mkdir)      (struct v_inode *this, struct v_dnode *dnode);
        int (*dir_lookup) (struct v_inode *this, struct v_dnode *dnode);
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
};

struct v_file{

    struct v_inode *inode;

    struct {
        void      *data;
        uint32_t  size;
        uint64_t  lb_addr;
        uint32_t  offset;
        int       dirty
    }buffer;

    struct {
        int (*write)    (struct v_file *this, void IN(*data), uint32_t size);
        int (*read)     (struct v_file *this, void OUT(*data), uint32_t size);
        int (*read_dir) (struct v_file *this, int dir_index);
        int (*seek)     (struct v_file *this, size_t offset);
        int (*rename)   (struct v_file *this, char *name);
        int (*close)    (struct v_file *this);
        int (*sync)     (struct v_file *this);
    }ops;
};

struct v_superblock{

    struct list_header  sb_list;
    int                 fs_id;
    bdev_t              dev;
    struct v_dnode      *root;
    struct filesystem   *fs;

    struct {

        uint32_t (*read_capacity)(struct v_superblock *this);
        uint32_t (*read_usage)(struct v_superblock *this);
    }ops;
};



void fsm_init();

void fsm_register(struct filesystem *fs);

struct filesystem *fsm_get(const char * fs_name);

#endif
















