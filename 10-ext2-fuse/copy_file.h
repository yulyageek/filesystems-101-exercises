#ifndef COPY_FILE_H
#define COPY_FILE_H

#include <sys/types.h>
/**
   Implement this function to copy the content of an inode @inode_nr
   to a file descriptor @out. @img is a file descriptor of an open
   ext2 image.

   It suffices to support single- and double-indirect blocks.

   If a copy was successful, return 0. If an error occurred during
   a read or a write, return -errno.
*/
int copy_file(int img, int inode_nr, char* out, off_t offset);

#endif