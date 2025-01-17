#define FUSE_USE_VERSION 31

#include <solution.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fuse.h>

static void *hello_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
        (void) conn;
        (void) cfg;
        return NULL;
}

static int hello_getattr(const char *path, struct stat *stbuf,
                          struct fuse_file_info *fi)
{
	(void) fi;
        memset(stbuf, 0, sizeof(struct stat));
 	if(strcmp(path, "/") == 0){
        	stbuf->st_mode = S_IFDIR;
		return 0;
	} 
	if(strcmp(path, "/hello") == 0){
        	stbuf->st_mode = S_IFREG | S_IRUSR;
        	stbuf->st_size = 400;
		return 0;
	}
        return -ENOENT;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	if(strcmp(path, "/hello") != 0)
                 return -ENOENT;
	struct fuse_context* fc = fuse_get_context();
	pid_t pid = fc->pid;
        size_t len = snprintf(NULL, 0, "hello, %d\n", pid);
        (void) fi;
	char* content = (char*) malloc (len);
	sprintf(content, "hello, %d\n", pid);
        if (offset < (long int)len) {
                if (offset + size > len)
                        size = len - offset;
                memcpy(buf, content + offset, size);
        } else
                size = 0;

	free(content);
        return size;
}

static int hello_open (const char *path, struct fuse_file_info *fi){
  if(strcmp(path, "/hello") != 0){
    return -ENOENT;
  }
  if((fi->flags & O_ACCMODE) != O_RDONLY)
    return -EROFS;
  return 0; 
} 

static int hello_create(const char* path, mode_t mode, struct fuse_file_info* fi){
  (void)path;
  (void)mode;
  (void)fi;
  return -EROFS;

}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi,
                         enum fuse_readdir_flags flags)
{
        (void) offset;
        (void) fi;
        (void) flags;

	if(strcmp(path, "/") == 0){
        	filler(buf, ".", NULL, 0, 0);
        	filler(buf, "..", NULL, 0, 0);
        	filler(buf, "hello", NULL, 0, 0);
		return 0;
	} 
	return -ENOENT;
}

static int hello_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	(void) buf;
	(void) size;
	(void) offset;
	(void) fi;
	if(strcmp(path, "/hello") != 0){}
                 //return -ENOENT;
	return -EROFS;
}

static int hello_write_buf(const char *path, struct fuse_bufvec *buf, off_t off, struct fuse_file_info *fi){
	(void) buf;
	(void) off;
	(void) fi;
	if(strcmp(path, "/hello") != 0){}
                 //return -ENOENT;
	return -EROFS;
}
static const struct fuse_operations hellofs_ops = {
	.init = hello_init,
	.getattr = hello_getattr,
        .readdir = hello_readdir,
        .read = hello_read,
        .open = hello_open,
        .create = hello_create,
        .write = hello_write,
        .write_buf = hello_write_buf,
};

int helloworld(const char *mntp)
{
	char *argv[] = {"exercise", "-f", (char *)mntp, NULL};
	return fuse_main(3, argv, &hellofs_ops, NULL);
}
