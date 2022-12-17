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
		free(name);
		fprintf(stderr, "Error: %s\n", strerror(errno));
		return -errno;
	}

	// ntfs_inode *ntfs_pathname_to_inode(ntfs_volume *vol, ntfs_inode *parent, const char *pathname);
	ntfs_inode *inode = ntfs_pathname_to_inode(volume, NULL, path);
	if (!inode) {
		free(name);
		// fprintf(stderr, "Error: %s\n", strerror(errno));
		return -errno;
	}

	//ntfs_attr *ntfs_attr_open(ntfs_inode *ni, const ATTR_TYPES type, ntfschar *name, u32 name_len);
	ntfs_attr *attr = ntfs_attr_open(inode, AT_DATA, NULL, 0);
	if (!attr) {
		free(name);
		return -errno;
	}

	char buffer[BUF_SIZE];
	s64 len;
	s64 offset;

	offset = 0;
	while(1){
		len = ntfs_attr_pread(attr, offset, BUF_SIZE, buffer);
		if (len == -1) {
			// fprintf(stderr, "read %ld\n", offset);
			free(name);
			return -errno;
		}
		if (len == 0)
			break;

		if (write(out, buffer, len) < len) {
			// fprintf(stderr, "read %ld\n", offset);
			free(name);
			return -errno;
		}
		offset += len;
	}
	ntfs_attr_close(attr);
	ntfs_inode_close(inode);
	ntfs_umount(volume, FALSE);
	free(name);
	return 0;
}