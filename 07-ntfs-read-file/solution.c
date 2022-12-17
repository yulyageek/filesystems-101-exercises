#include <solution.h>

#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// avoid redefinition of ‘struct timespec’
#define st_mtime
#define HAVE_TIME_H

#include <ntfs-3g/types.h>
#include <ntfs-3g/attrib.h>
#include <ntfs-3g/volume.h>
#include <ntfs-3g/dir.h>

#define BUF_SIZE 4096

char *get_name(int fd){
	(void) fd;
	char fd_link[PATH_MAX];
	char* name = (char*)malloc(PATH_MAX);
	sprintf(fd_link, "/proc/self/fd/%d", fd);
	int ret = readlink(fd_link, name, NAME_MAX);
	if (ret < 0){
		free(name);
		return NULL;
	}
	name[ret] = '\0';
	return name;
}

int dump_file(int img, const char *path, int out)
{
	// ntfs_volume *ntfs_mount(const char *name, ntfs_mount_flags flags);
	char *name = get_name(img);
	if (!name)
		return -errno;

	ntfs_volume *volume = ntfs_mount(name, NTFS_MNT_RDONLY);
	if (!volume){
		int ret = -errno;
		// fprintf(stderr, "Error: %s\n", strerror(errno));
		free(name);
		return ret;
	}

	// ntfs_inode *inode = ntfs_pathname_to_inode(volume, NULL, path);
	// if (!inode) {
	// 	// запомнить errno
	// 	int ret = -errno;
	// 	// fprintf(stderr, "Error: %s\n", strerror(errno));
	// 	free(name);
	// 	ntfs_umount(volume, FALSE);
	// 	return ret;
	// }
	ntfs_inode *parent_inode = NULL;
	ntfs_inode *inode = NULL;
	char* start = (char*)path;
	char* end;
	u32 entry_name_len;
	char *rel_name = (char*) malloc(strlen(path));
	while(1){
		end = index(start, '/');
		if (!end){
			entry_name_len = path + strlen(path) - start;
		} else {
			entry_name_len = end - start + 1;
		}
		memcpy(rel_name, start, entry_name_len);
		rel_name[entry_name_len] = '\0';
		inode = ntfs_pathname_to_inode(volume, parent_inode, rel_name);
		ntfs_inode_close(parent_inode);
		if (!inode) {
			// запомнить errno
			int ret = -errno;
			free(name);
			free(rel_name);
			// fprintf(stderr, "Error: %s\n", strerror(errno));
			ntfs_umount(volume, FALSE);
			return ret;
		}
		parent_inode = inode;
		if(!end){
			break;
		}
		if (!(inode->mrec->flags & MFT_RECORD_IS_DIRECTORY)){
			free(name);
			free(rel_name);
			// fprintf(stderr, "Error: %s\n", strerror(errno));
			ntfs_inode_close(inode);
			ntfs_umount(volume, FALSE);
			return -ENOTDIR;
		}
		start = end + 1;
	}
	free(rel_name);

	//ntfs_attr *ntfs_attr_open(ntfs_inode *ni, const ATTR_TYPES type, ntfschar *name, u32 name_len);
	ntfs_attr *attr = ntfs_attr_open(inode, AT_DATA, NULL, 0);
	if (!attr) {
		int ret = -errno;
		// fprintf(stderr, "Error: %s\n", strerror(errno));
		free(name);
		ntfs_inode_close(inode);
		ntfs_umount(volume, FALSE);
		return ret;
	}

	char buffer[BUF_SIZE];
	s64 len;
	s64 offset;

	offset = 0;
	while(1){
		len = ntfs_attr_pread(attr, offset, BUF_SIZE, buffer);
		if (len == -1) {
			// fprintf(stderr, "read %ld\n", offset);
			int ret = -errno;
			ntfs_attr_close(attr);
			ntfs_inode_close(inode);
			ntfs_umount(volume, FALSE);
			free(name);
			return ret;
		}
		if (len == 0)
			break;

		if (write(out, buffer, len) < len) {
			// fprintf(stderr, "read %ld\n", offset);
			int ret = -errno;
			ntfs_attr_close(attr);
			ntfs_inode_close(inode);
			ntfs_umount(volume, FALSE);
			free(name);
			return ret;
		}
		offset += len;
	}
	ntfs_attr_close(attr);
	ntfs_inode_close(inode);
	ntfs_umount(volume, FALSE);
	free(name);
	return 0;
}