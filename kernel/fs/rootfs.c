
#include <fs/rootfs.h>
#include <fs/fs.h>
#include <clock.h>
#include <libc/string.h>
#include <mm/cake.h>
#include <mm/valloc.h>


static struct rootfs_node *fs_root;
static struct cake_pile   *rtfs_pile;

int __rootfs_dir_lookup(struct v_inode *inode, struct v_dnode *dnode);
int __rootfs_fileopen(struct v_inode *inode, struct v_file *file);
int __rootfs_mount(struct v_superblock *sb, struct v_dnode *mount);
int __rootfs_iterate_dir(struct v_file *file, struct dir_context *dc);

struct rootfs_node *__rootfs_get_node
    (struct rootfs_node *parent, struct hash_str *name);
struct v_inode *__rootfs_create_inode
    (struct rootfs_node *node);


void rootfs_init(){

    rtfs_pile =
      cake_pile_create("rtfs_node", sizeof(struct rootfs_node), 1, 0);

    struct filesystem *rtfs = vzalloc(sizeof(struct filesystem));
    rtfs->fs_name = HASH_STR("rootfs", 6);
    rtfs->mount   = __rootfs_mount;
    fsm_register(rtfs);

    fs_root = rootfs_dir_node(NULL, NULL, 0);
}

struct rootfs_node * __rootfs_new_node(
    struct rootfs_node *parent, const char *name, int len){

    struct rootfs_node *node = cake_piece_grub(rtfs_pile);
    memset(node, 0, sizeof(*node));

    node->name = HASH_STR(name, len);
    hash_str_rehash(&node->name, HSTR_FULL_HASH);
    list_init_head(&node->children);

    if(parent){
        list_append(&parent->children, &node->siblings);
    }
    return node;
}

struct rootfs_node *rootfs_file_node(
        struct rootfs_node *parent,
        const char *name,
        int len){

    struct rootfs_node *node = __rootfs_new_node(parent, name, len);
    node->itype = VFS_INODE_TYPE_FILE;

    struct v_inode *inode =  __rootfs_create_inode(node);
    node->inode           =  inode;
    return node;
}

struct rootfs_node *rootfs_dir_node(
        struct rootfs_node *parent,
        const char *name,
        int len){

    struct hash_str hs = HASH_STR(name, len);
    hash_str_rehash(&hs, HSTR_FULL_HASH);
    struct rootfs_node *res = __rootfs_get_node(parent, &hs);

    if(res) return res;

    struct rootfs_node *node =  __rootfs_new_node(parent, name, len);
    struct rootfs_node *dot  =  __rootfs_new_node(node, ".", 1);
    struct rootfs_node *ddot =  __rootfs_new_node(node, "..", 2);
    
    
    node->itype =  VFS_INODE_TYPE_DIR;
    dot->itype  =  VFS_INODE_TYPE_DIR;
    ddot->itype =  VFS_INODE_TYPE_DIR;
    
    node->inode =  __rootfs_create_inode(node);
    dot->inode  =  node->inode;
    ddot->inode =  parent ? parent->inode : node->inode;

    node->fops.read_dir = __rootfs_iterate_dir;
    
    return node;
}

struct rootfs_node *rootfs_toplevel_node (const char *name, int len){

    return rootfs_dir_node(fs_root, name, len);
}

int __rootfs_mkdir(struct v_inode *inode, struct v_dnode *dnode){

    struct rootfs_node *node = inode->data;
    if(!(node->itype & VFS_INODE_TYPE_DIR)){
        return ENOTSUP;
    }

    struct rootfs_node *new_node = 
        rootfs_dir_node(node, dnode->name.value, dnode->name.len);
    dnode->inode = new_node->inode;

    return 0;
}

struct v_inode *__rootfs_create_inode(struct rootfs_node *node){
    
    struct v_inode *inode = vfs_inode_alloc();

    memset(inode, 0, sizeof(*inode));

    inode->itype          =  node->itype;
    inode->data           =  node;
    inode->ops.dir_lookup =  __rootfs_dir_lookup;
    inode->ops.open       =  __rootfs_fileopen;
    inode->ops.mkdir      =  __rootfs_mkdir;
    return inode;
}

int __rootfs_mount(struct v_superblock *sb, struct v_dnode *mnt){

    mnt->inode = fs_root->inode;

    if(mnt->parent && mnt->parent->inode){

        struct hash_str ddot_name     =
            HASH_STR("..", 2);
        struct rootfs_node *root_ddot =
            __rootfs_get_node(fs_root, &ddot_name);
        root_ddot->inode              =
            mnt->parent->inode;
    }
    return 0;
}

struct rootfs_node *__rootfs_get_node(
    struct rootfs_node *parent, struct hash_str *name){

    if(!parent)return NULL;

    struct rootfs_node *pos, *n;
    list_for_each(pos, n, &parent->children, siblings){

        if(pos->name.hash == name->hash){
            return pos;
        }
    }
    return 0;
}

int __rootfs_dir_lookup(struct v_inode *inode, struct v_dnode *dnode){
    
    if(!(inode->itype & VFS_INODE_TYPE_DIR)){
        return ENOTSUP;
    }
    struct rootfs_node *child_node =
        __rootfs_get_node(inode->data, &dnode->name);

    if(child_node){
        dnode->inode = child_node->inode;
        return 0;
    }
    return ENOENT;
}

int __rootfs_fileopen(struct v_inode *inode, struct v_file *file){

    struct rootfs_node *node = inode->data;

    if(node){
        file->ops   = node->fops;
        file->inode = inode;
        return 0;
    }
    return ENOTSUP;
}

int __rootfs_iterate_dir(struct v_file *file, struct dir_context *dc){

    struct rootfs_node *node = file->inode->data;
    int counter = 0;
    struct rootfs_node *pos, *n;
    list_for_each(pos, n, &node->children, siblings){

        if(counter++ >= dc->index){
            dc->index = counter;
            dc->read_complete_callback
                (dc, pos->name.value, pos->name.len, pos->itype);
            return 0;
        }
    }
    return 1;
}

void rootfs_rm_node(struct rootfs_node *node){

    if(node->itype & VFS_INODE_TYPE_DIR){

        struct rootfs_node *dir1 =
            __rootfs_get_node(node, &vfs_dot);
        struct rootfs_node *dir2 =
            __rootfs_get_node(node, &vfs_ddot);
        vfs_inode_free(dir1->inode);
        vfs_inode_free(dir2->inode);
        cake_piece_release(rtfs_pile, dir1);
        cake_piece_release(rtfs_pile, dir2);
    }
    list_delete(&node->siblings);
    vfs_inode_free(node->inode);
    cake_piece_release(rtfs_pile, node);

}






