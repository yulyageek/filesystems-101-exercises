#define FUSE_USE_VERSION 31

#include <solution.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fuse.h>
#include <copy_file.h>
#include <read_dir.c>
#include <ext2.h>

static int ext2_img;

static void *ext2_init(struct fuse_conn_info *conn)
{
        (void) conn;
        return NULL;
}

static int ext2_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
        (void) path;
        (void) buf;
        (void) size;
        (void) offset;
        (void) fi;
        return -EROFS;
}

static int ext2_create(const char* path, mode_t mode, struct fuse_file_info* fi){
        (void)path;
        (void)mode;
        (void)fi;
        return -EROFS;
}

static int ext2_open(const char *path, struct fuse_file_info *ffi)
{
        int inode = get_inode_nr(ext2_img, path);
        if (inode < 0){
                return inode;
        }
        if ((ffi->flags & O_ACCMODE) != O_RDONLY) {
    	        return -EROFS;
        }
	return 0;
}

static int ext2_getattr(const char *path, struct stat *stbuf)
{
        memset(stbuf, 0, sizeof(struct stat));
        int ret = get_inode_nr(ext2_img, path);
        if (ret < 0){
                return ret;
        }
        int inode_nr = ret;
        struct ext2_inode inode;
        ret = read_inode(ext2_img, &inode, inode_nr);\
        if (ret < 0){
                return ret;
        }
        stbuf->st_mode = inode.i_mode;
        stbuf->st_nlink = inode.i_links_count;
        stbuf->st_uid = inode.i_uid;
        stbuf->st_gid = inode.i_gid;
        stbuf->st_size = inode.i_size;
        stbuf->st_atime = inode.i_atime;
        return 0;
}

static int ext2_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
        (void) buf;
        (void) size;
        (void) offset;
        (void) fi;
        int inode = get_inode_nr(ext2_img, path);
        if (inode < 0){
                return inode;
        }
        return copy_file(ext2_img, inode, buf);
}

static int ext2_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi
                         /*enum fuse_readdir_flags flags*/)
{
        (void) offset;
        (void) fi;
        int inode_nr = get_inode_nr(ext2_img, path);
        if (inode_nr < 0){
                return inode_nr;
        }
	return dump_dir(ext2_img, inode_nr, filler, buf);
}

static const struct fuse_operations ext2_ops = {
	.init = ext2_init, // OK
	.getattr = ext2_getattr, //OK
        .readdir = ext2_readdir,
        .read = ext2_read, // OK
        .open = ext2_open, // OK
        .create = ext2_create, // OK
        .write = ext2_write, // OK
};

int ext2fuse(int img, const char *mntp){
        ext2_img = img;
	char *argv[] = {"exercise", "-f", (char *)mntp, NULL};
	return fuse_main(3, argv, &ext2_ops, NULL);
}