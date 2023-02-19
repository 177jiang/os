#ifndef __jyos_rootfs_h_
#define __jyos_rootfs_h_

#include <fs/fs.h>

struct rootfs_node{

    struct v_inode     *inode;
    struct hash_str     name;
    uint32_t            itype;

    struct list_header  children;
    struct list_header  siblings;

    struct v_file_ops   fops;
    void                *data;
};

void rootfs_init();

struct rootfs_node* rootfs_file_node(
    struct rootfs_node *parent, const char *name, int name_len);

struct rootfs_node* rootfs_dir_node(
    struct rootfs_node *parent, const char *name, int name_len);

struct rootfs_node *rootfs_toplevel_node(
        const char *name, int name_len);

int rootfs_rm_node(struct rootfs_node *node);

#endif
