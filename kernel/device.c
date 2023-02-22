#include <device.h>
#include <libc/stdio.h>

#include <fs/rootfs.h>

#include <mm/valloc.h>

struct list_header dev_list;

static struct rootfs_node *dev_root;

int __dev_read(struct v_file *file, void *buffer, size_t len, size_t pos);

int __dev_write(struct v_file *file, void *buffer, size_t len, size_t pos);


void device_init(){

    dev_root = rootfs_toplevel_node("dev", 3);
    list_init_head(&dev_list);
}

struct device *device_add(struct device *parent,
                void *underlay, char *name_fmt, ...){
    
    struct device *dev = vzalloc(sizeof(struct device));

    va_list args;
    va_start(args, name_fmt);

    int strlen = 
        vsnprintf_(dev->dev_name, DEVICE_NAME_MAXLEN, name_fmt, args);

    dev->name     =  HASH_STR(dev->dev_name, strlen);
    dev->underlay =  underlay;
    dev->parent   =  parent;

    hash_str_rehash(&dev->name, HSTR_FULL_HASH);
    list_append(&dev_list, &dev->dev_list);


    struct rootfs_node *dev_node =
        rootfs_file_node(dev_root, dev->dev_name, strlen);
    dev_node->data       =  dev;
    dev_node->fops.read  =  __dev_read;
    dev_node->fops.write =  __dev_write;

    dev->fs_node    = dev_node;

    va_end(args);
    return dev;
}

int __dev_read(struct v_file *file, void *buffer, size_t len, size_t pos){

    struct rootfs_node *dev_node = file->inode->data;
    struct device *dev           = dev_node->data;

    if(!dev->read) return ENOTSUP;
    return dev->read(dev, buffer, pos, len);
}

int __dev_write(struct v_file *file, void *buffer, size_t len, size_t pos){

    struct rootfs_node *dev_node = file->inode->data;
    struct device *dev           = dev_node->data;

    if(!dev->write) return ENOTSUP;
    return dev->write(dev, buffer, pos, len);
}

void device_remove(struct device *dev){

    list_delete(&dev->dev_list);
    rootfs_rm_node((struct rootfs_node *)dev->fs_node);
    vfree(dev);
}






