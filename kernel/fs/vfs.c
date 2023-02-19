#include <libc/string.h>

#include <fs/fs.h>
#include <fs/dirent.h>
#include <fs/foptions.h>

#include <mm/cake.h>
#include <mm/page.h>
#include <mm/valloc.h>

#include <spike.h>
#include <process.h>
#include <syscall.h>


#define PATH_DELIM                  '/'
#define DNODE_HASHTABLE_BITS        10
#define DNODE_HASHTABLE_SIZE        (1 << DNODE_HASHTABLE_BITS)
#define DNODE_HASK_MASK             (DNODE_HASHTABLE_SIZE - 1)
#define DNODE_HASHBITS              (32 - DNODE_HASHTABLE_BITS)


static struct cake_pile             *dnode_pile;
static struct cake_pile             *inode_pile;
static struct cake_pile             *file_pile;
static struct cake_pile             *fd_pile;
static struct cake_pile             *superblock_pile;

static struct v_superblock          *root_sb;
static struct hash_bucket           *dnode_cache;

struct hash_str vfs_ddot  =  HASH_STR("..", 2);
struct hash_str vfs_dot   =  HASH_STR(".", 1);
struct hash_str vfs_empty =  HASH_STR("", 0);

static int fs_id = 0;

void vfs_init(){

    dnode_pile      =
        cake_pile_create("dnode_cache", sizeof(struct v_dnode), 1, 0);
    inode_pile      =
        cake_pile_create("inode_cache", sizeof(struct v_inode), 1, 0);
    file_pile       =
        cake_pile_create("file_cache", sizeof(struct v_file), 1, 0);
    fd_pile         =
        cake_pile_create("fd_cache", sizeof(struct v_fd), 1, 0);
    superblock_pile =
        cake_pile_create("sb_cache", sizeof(struct v_superblock), 1, 0);
    dnode_cache     =
        vzalloc(DNODE_HASHTABLE_SIZE * sizeof(struct hash_bucket));

    hash_str_rehash(&vfs_dot, HSTR_FULL_HASH);
    hash_str_rehash(&vfs_ddot, HSTR_FULL_HASH);

    root_sb        =  vfs_sb_alloc();
    root_sb->root  =  vfs_dnode_alloc();
}

inline struct hash_bucket *__dcache_get_bucket(
        struct v_dnode *parent, unsigned int hash){

    hash += (uint32_t)parent;
    hash = hash ^ (hash >> DNODE_HASHBITS);
    return dnode_cache + (hash & DNODE_HASK_MASK);
}

struct v_dnode *vfs_dcache_lookup(struct v_dnode *parent,
                                 struct hash_str *hstr){

    if(!hstr->len || HASH_SEQ(hstr, &vfs_dot)){
        return parent;
    }
    if(HASH_SEQ(hstr, &vfs_ddot)){
        return parent->parent ? parent->parent : parent;
    }

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
             struct hash_str *compoent,
             int walk_options){

    int error = 0;
    int i = 0, j = 0;
    if(path[0] == PATH_DELIM || !start){
        if((walk_options & VFS_WALK_FSRELATIVE) && start){
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
    char cur  = path[i++], next;

    while(cur){

        next = path[i++];
        if(cur != PATH_DELIM){
            if(j >= VFS_NAME_MAXLEN - 1){
                return ENAMETOOLONG;
            }
            if(!VFS_VALID_CHAR(cur)){
                return VFS_EINVLD;
            }
            tname[j++] = cur;

            if(next) goto __cont;
        }

        if(next == PATH_DELIM)goto __cont;

        tname[j] =  0;
        hs.len   =  j;
        hash_str_rehash(&hs, HSTR_FULL_HASH);

        if(!next && (walk_options & VFS_WALK_PARENT)){

            if(compoent){
                compoent->hash =  hs.hash;
                compoent->len  =  j;
                strcpy(compoent->value, hs.value);
            }
            break;
        }


        dnode = vfs_dcache_lookup(current_level, &hs);

        if(!dnode){

            dnode            =  vfs_dnode_alloc();
            dnode->name      =  HASH_STR(valloc(VFS_NAME_MAXLEN), j);
            dnode->name.hash =  hs.hash;
            strcpy(dnode->name.value, hs.value);

            if(current_level->inode){

                error = current_level->inode->ops.dir_lookup(
                            current_level->inode, dnode);
            }

            int create = (walk_options  & VFS_WALK_MKPARENT);

            if((error == ENOENT) && create){
                if(!current_level->inode->ops.mkdir){
                    error = VFS_ENOFS;
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
__cont:
        cur = next;
    };

    *dentry = current_level;
    return error;

__error:
    vfree(dnode->name.value);
    vfs_dnode_free(dnode);
    *dentry = NULL;
    return error;
}

int vfs_open(struct v_dnode *dnode, struct v_file **file){

    if(!dnode->inode || !dnode->inode->ops.open){
        return ENOTSUP;
    }

    struct v_file *f = cake_piece_grub(file_pile);
    memset(f, 0, sizeof(*f));
    f->dnode = dnode;
    f->inode = dnode->inode;
    ++dnode->inode->open_count;

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
        return ENOTSUP;
    }

    int error = file->ops.close(file);

    if(!error){
        cake_piece_release(file_pile, file);
        if(file->inode->open_count){
            --file->inode->open_count;
        }
    }
    return error;
}

int vfs_fsync(struct v_file *file){

    if(!file || !file->ops.sync){
        return ENOTSUP;
    }
    int error = file->ops.sync(file);
    if(!error || file->inode->ops.sync){
        error = file->inode->ops.sync(file->inode);
    }
    return error;
}

int vfs_link(struct v_dnode *to_link, struct v_dnode *name){

    int error;
    if(to_link->super_block->root != name->super_block->root){

        error = EXDEV;
    }else if(!to_link->inode->ops.link){
        error = ENOTSUP;
    }else{
        error = to_link->inode->ops.link(to_link->inode, name);
        if(!error){
            name->inode = to_link->inode;
            ++to_link->inode->link_count;
        }
    } 
    return error;
}

int vfs_mount(const char *target, const char *fs_name, bdev_t device){

    struct v_dnode *mnt = 0;
    int error = vfs_walk(NULL, target, &mnt, NULL, 0);

    if(!error){
        error = vfs_mount_at(fs_name, device, mnt);
    }
    return error;
}

int vfs_mount_at(const char *fs_name,
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

int vfs_unmount(const char *target){

    struct v_dnode *mnt = 0;
    int error = vfs_walk(NULL, target, &mnt, NULL, 0);

    if(!error){
        error = vfs_unmount_at(mnt);
    }
    return error;
}

int vfs_unmount_at(struct v_dnode *mnt_point){

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
    dnode->name = vfs_empty;
    return dnode;
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

int vfs_fdslot_alloc(int *fd){

    for(size_t i=0; i<VFS_FD_MAX; ++i){

        if(!__current->fdtable->fds[i]){
            *fd = i;
            return 0;
        }
    }
    return EMFILE;
}

#define FLOCATE_CREATE_EMPTY 1
int __vfs_try_locate_file(const char *path, struct v_dnode ** fdir, 
                         struct v_dnode **file, int options){

    char name_str[VFS_NAME_MAXLEN];
    struct hash_str name = HASH_STR(name_str, 0);

    int error = vfs_walk(NULL, path, fdir, &name, VFS_WALK_PARENT);
    if(error) return error;

    error = vfs_walk(*fdir, name.value, file, NULL, 0);
    if(error == ENOENT && (options & FLOCATE_CREATE_EMPTY)){

        struct v_dnode *file_new = vfs_dnode_alloc();
        file_new->name =
            HSTR_SET(valloc(VFS_NAME_MAXLEN), name.len, name.hash);
        strcpy(file_new->name.value, name.value);
        *file = file_new;

        list_append(&(*fdir)->children, &file_new->siblings);
    }

    return error;
}

int __vfs_do_open(struct v_file **out_file, const char *path, int options){

    struct v_dnode *parent, *file;
    struct v_file *opened_file = 0;

    int error = __vfs_try_locate_file(path, &parent, &file, 0);
    if(error != ENOENT && (options & F_CREATE)){
        error = parent->inode->ops.create(parent->inode, opened_file);
    }else if(!error){
        error = vfs_open(file, &opened_file);
    }

    *out_file = opened_file;
    return error;

}
__DEFINE_SYSTEMCALL_2(int, open,
                      const char *, path,
                      int, options){

    struct v_file *opened_file;
    int fd;
    int error = __vfs_do_open(&opened_file, path, options);
    if(!error && !(error = vfs_fdslot_alloc(&fd))){

        struct v_fd *vfd = vzalloc(sizeof(struct v_fd));
        vfd->file = opened_file;
        vfd->pos  = opened_file->inode->fsize & -(!!(options & F_APPEND));
        __current->fdtable->fds[fd] = vfd;
        return fd;
    }
    __current->k_status = error;
    return SYSCALL_ESTATUS(error);
}

#define INVALID_FD(fd, vfd)           \
    (fd < 0 || fd >= VFS_FD_MAX || !(vfd = __current->fdtable->fds[fd]))

__DEFINE_SYSTEMCALL_1(int, close,
                      int, fd){

    struct v_fd *vfd;
    int error = 0;
    if(INVALID_FD(fd, vfd)){

        error = EBADF;
    }else if(!(error = vfs_close(vfd->file))){

        vfree(vfd);
        __current->fdtable->fds[fd] = 0;
    }

    __current->k_status = error;
    return SYSCALL_ESTATUS(error);
}

void __vfs_readdir_callback(struct dir_context *dctx,
                            const char *name,
                            const int  len, 
                            const int dtype){

    struct dirent *dent = dctx->cb_data;
    strncpy(dent->d_name, name, len);
    dent->d_nlen = len;
    dent->d_type = dtype;
}

int vfs_readlink
    (struct v_dnode *dnode, char *buf, size_t size, int dep){

    if(!dnode) return 0;
    if(dep > 64) return ELOOP;

    size_t len = vfs_readlink(dnode->parent, buf, size, dep + 1);

    if(len >= size) return len;

    size_t csize = MIN(size-len, dnode->name.len);
    strncpy(buf + len, dnode->name.value, csize);

    len += csize;

    if(len < size) buf[len++] = PATH_DELIM;
    return len;
}

int __vfs_do_unlink(struct v_inode *inode){

    int error;
    if(inode->open_count){

        error = EBUSY;
    }else if(!(inode->itype & VFS_INODE_TYPE_DIR)){

        error = inode->ops.unlink(inode);
        if(!error) --inode->link_count;
    }else{

        error =  EISDIR;
    }
    return error;
}

__DEFINE_SYSTEMCALL_2(int, readdir,
                      int, fd,
                      struct dirent *, dent){
    
    struct v_fd *vfd;
    int error = 0;
    if(INVALID_FD(fd, vfd)){

        error = EBADF;
    }else if(!(vfd->file->inode->itype & VFS_INODE_TYPE_DIR)){

        error = ENOTDIR;
    }else{

        struct dir_context dctx = (struct dir_context){
            .cb_data = dent,
            .index   = dent->d_offset,
            .read_complete_callback = __vfs_readdir_callback
        };

        switch(dent->d_offset){

            case 0:
                __vfs_readdir_callback(&dctx, vfs_dot.value, 1, 0);
                break;
            case 1:
                __vfs_readdir_callback(&dctx, vfs_ddot.value, 2, 0);
                break;
            default:
                dctx.index -= 2;
                error = vfd->file->ops.read_dir(vfd->file, &dctx);

                if(error) goto __done;
                break;
        }
        ++dent->d_offset;
    }

__done:
    __current->k_status = error;
    return SYSCALL_ESTATUS(error);
}

__DEFINE_SYSTEMCALL_1(int, mkdir,
                      const char *, path){

    struct v_dnode *parent, *dir;
    struct hash_str compoent = HASH_STR(valloc(VFS_NAME_MAXLEN), 0);

    int error =
        vfs_walk(root_sb->root, path, &parent, &compoent, VFS_WALK_PARENT);

    if(error) goto __done;


    if((parent->super_block->fs->type & FSTYPE_ROFS)){

        error = ENOTSUP;
    } else if(!parent->inode->ops.mkdir){

        error = ENOTSUP;
    }else if(!(parent->inode->itype & VFS_INODE_TYPE_DIR)){

        error = ENOTDIR;
    }else{

        dir = vfs_dnode_alloc();
        dir->name = compoent;
        if(!(error = parent->inode->ops.mkdir(parent->inode, dir))){

            list_append(&parent->children, &dir->siblings);
        }else{

            vfs_dnode_free(dir);
            vfree(compoent.value);
        }
    }
__done:
    __current->k_status = error;
    return SYSCALL_ESTATUS(error);
}

__DEFINE_SYSTEMCALL_3(int, read,
                      int,    fd,
                      void *, buf,
                      unsigned int, count){

    int error = 0;
    struct v_fd *vfd;

    if(INVALID_FD(fd, vfd)){

        error = EBADF;
    }else{

        struct v_file *file =  vfd->file;
        file->f_pos         =  vfd->pos;
        error               =  file->ops.read(file, buf, count);
        if(error >=  0){
            vfd->pos += error;
        }
    }
    __current->k_status = error;
    return SYSCALL_ESTATUS(error);
}

__DEFINE_SYSTEMCALL_3(int, write,
                      int,    fd,
                      void *, buf,
                      unsigned int, count){
    int error = 0;
    struct v_fd *vfd;

    if(INVALID_FD(fd, vfd)){

        error = EBADF;
    }else{

        struct v_file *file =  vfd->file;
        file->f_pos         =  vfd->pos;
        error               =  file->ops.write(file, buf, count);
        if(error >=  0){
            vfd->pos += error;
        }
    }
    __current->k_status = error;
    return SYSCALL_ESTATUS(error);
}

__DEFINE_SYSTEMCALL_3(int, lseek,
                      int,    fd,
                      int, offset,   
                      int, options){

    int error = 0;
    struct v_fd *vfd;

    if(INVALID_FD(fd, vfd)){

        error = EBADF;
    }else{

        size_t fpos = vfd->file->f_pos;
        switch(options){

            case FSEEK_CUR:
                fpos = (size_t)(fpos + offset);
                break;
            case FSEEK_END:
                fpos = (size_t)(vfd->file->inode->fsize + offset);
                break;
            case FSEEK_SET:
                fpos = (size_t)offset;
                break;
            default:
                break;
        }

        vfd->pos = fpos;
    }
    __current->k_status = error;
    return SYSCALL_ESTATUS(error);
}


__DEFINE_SYSTEMCALL_3(int, readlink,
                      const char *, path,
                      char *, buf,
                      size_t, size){
    int error;

    struct v_dnode *dnode;
    if(!(error = vfs_walk(NULL, path, &dnode, NULL, 0))){

        error = vfs_readlink(dnode, buf, size, 0);
    }

    if(error >= 0) return error;

    __current->k_status = error;
    return SYSCALL_ESTATUS(error);
}

__DEFINE_SYSTEMCALL_4(int, readlinkat,
                      int, dirfd,
                      const char *, path,
                      char *, buf,
                      size_t, size){
    int error;
    struct v_fd *vfd;
    if(INVALID_FD(dirfd, vfd)){
        error = EBADF;
    }else{
        struct v_dnode *dnode;
        error = vfs_walk(vfd->file->dnode, path, &dnode, NULL, 0);
        if(!error){
            error = vfs_readlink(dnode, buf, size, 0);
        }
    }

    if(error >= 0) return error;


    __current->k_status = error;
    return SYSCALL_ESTATUS(error);
}

__DEFINE_SYSTEMCALL_1(int, rmdir,
                      const char *, path){

    int error = 0;
    do{
        struct v_dnode *dnode;
        error = vfs_walk(NULL, path, &dnode, NULL, 0);
        if(error) break;

        if((dnode->super_block->fs->type & FSTYPE_ROFS)){
            error = EROFS;
            break;
        }
        if(dnode->inode->open_count){
            error = EBUSY;
            break;
        }
        if(dnode->inode->itype & VFS_INODE_TYPE_DIR){
            error = dnode->inode->ops.rmdir(dnode->inode);
            break;
        }
        error = ENOTDIR;
    }while(0);

    __current->k_status = error;
    return SYSCALL_ESTATUS(error);
}

__DEFINE_SYSTEMCALL_1(int, unlink,
                      const char *, path){
    
    int error = 0;
    do{
        struct v_dnode *dnode;
        error = vfs_walk(NULL, path, &dnode, NULL, 0);
        if(error) break;

        if((dnode->super_block->fs->type & FSTYPE_ROFS)){
            error = EROFS;
            break;
        }
        error = __vfs_do_unlink(dnode->inode);
    }while(0);

    __current->k_status = error;
    return SYSCALL_ESTATUS(error);
}
    
__DEFINE_SYSTEMCALL_2(int, unlinkat,
                      int, fd,
                      const char *, path){
    int error;
    struct v_fd *vfd;
    if(INVALID_FD(fd, vfd)){
        error = EBADF;
    }else{
        struct v_dnode *dnode;
        error = vfs_walk(vfd->file->dnode, path, &dnode, NULL, 0);
        if(!error){
            error = __vfs_do_unlink(dnode->inode);
        }
    }
    __current->k_status = error;
    return SYSCALL_ESTATUS(error);
}

__DEFINE_SYSTEMCALL_2(int, link,
                      const char *, oldpath,
                      const char *, newpath){

    struct v_dnode *parent, *to_link, *name_parnet, *name = 0;

    int error = __vfs_try_locate_file(oldpath, &parent, &to_link, 0);

    if(!error){
        error = __vfs_try_locate_file(
                newpath,
                &name_parnet,
                &name,
                FLOCATE_CREATE_EMPTY
        );
        if(!error){
            error = EEXIST;
        }else if(name){
            error = vfs_link(to_link, name);
        }
    }
    __current->k_status = error;
    return SYSCALL_ESTATUS(error);
}

__DEFINE_SYSTEMCALL_1(int, fsync,
                      int, fildes){

    int error;
    struct v_fd *vfd;
    if(INVALID_FD(fildes, vfd)){
        error = EBADF;
    }else{
        error = vfs_fsync(vfd->file);
    }
    __current->k_status = error;
    return SYSCALL_ESTATUS(error);
}





















