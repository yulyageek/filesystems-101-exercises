#include <ext2.h>
#include <scan_dir.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <linux/types.h>

int get_inode_nr(int img, const char *path){
	if (path[0] != '/'){
		return -ENOENT;
	}
	int ret = 0;
	char entry_name[MAX_LEN];
	__u8 entry_type;
	int inode_nr = 2; //root_inode
	char* start = (char*)path + 1;
	char* end;
	int entry_name_len;
	while(1){
		end = index(start, '/');
		if(end == NULL){
			entry_type = EXT2_FT_REG_FILE;
			entry_name_len = path + strlen(path) - start;
		} else{
			entry_type = EXT2_FT_DIR;
			entry_name_len = end - start;
		} 
		memcpy(entry_name, start, entry_name_len);
		entry_name[entry_name_len] = '\0';
		ret = scan_dir(img, inode_nr, entry_name, entry_type);
		if (ret < 0){
			return ret;
		}
        inode_nr = ret;
        if(end == NULL){
            break;
        }
		start = end + 1;
	}
	return inode_nr;
}

int read_inode(int img, struct ext2_inode *inode, int inode_nr){
    struct ext2_super_block  sb;
	ssize_t len = pread(img, &sb, sizeof(sb), SUPERBLOCK_OFFSET);
	if(len == -1){
		return -errno;
	}
	unsigned int block_size = 1024 << sb.s_log_block_size;
	int block_group_nr = (inode_nr - 1) / sb.s_inodes_per_group;

	struct ext2_group_desc gd;
	int read_offset = block_size * (sb.s_first_data_block + 1) + block_group_nr * sizeof(gd);
	len = pread(img, &gd, sizeof(gd), read_offset);
	if(len == -1){
		return -errno;
	}

	int inode_in_group = (inode_nr - 1) % sb.s_inodes_per_group;

	read_offset = block_size * gd.bg_inode_table + inode_in_group * sb.s_inode_size;
	len = pread(img, inode, sizeof(*inode), read_offset);
	if(len == -1){
		return -errno;
	}
    return 0;
}