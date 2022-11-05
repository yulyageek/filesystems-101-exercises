#include <solution.h>
#include <ext2.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <linux/types.h>

static char* block_buf = NULL;
static __le32* single_inderect_block_buf = NULL;
static __le32* double_inderect_block_buf = NULL;
char entry_name[MAX_LEN];
static __u32 block_size;

__attribute__((destructor)) void free_all(void){
	if (block_buf != NULL)
		free(block_buf);
	if(single_inderect_block_buf != NULL)
		free(single_inderect_block_buf);
	if(double_inderect_block_buf != NULL)
		free(double_inderect_block_buf);		
}

int read_direct_block(int img, __le32 block_nr){
	__u32 offset = block_nr * block_size;
	struct ext2_dir_entry_2 entry;
	while(1){
		ssize_t len = pread(img, &entry, sizeof(entry), offset);
		if(len == -1){
			return -errno;
		}
		size_t name_len = entry.name_len;
		len = pread(img, entry_name, name_len, offset + sizeof(entry));
		if(len == -1){
			return -errno;
		}
		if (entry.inode == 0){
			return 0;
		}
		entry_name[name_len] = '\0';
		if (entry.file_type == EXT2_FT_REG_FILE){
			//printf("%d f %s\n", entry.inode, entry_name);
			report_file(entry.inode, 'f', entry_name);
		}
		if (entry.file_type == EXT2_FT_DIR){
			//printf("%d d %s\n", entry.inode, entry_name);
			report_file(entry.inode, 'd', entry_name);
		}
		offset += entry.rec_len;
	}
	return 0;
}

int read_single_indirect_block(int img, __le32 block_nr){
	if (block_nr == 0){
		return 0;
	}
	__u32 offset = block_nr * block_size;
	ssize_t len = pread(img, single_inderect_block_buf, block_size, offset);
	if(len == -1){
		return -errno;
	}
	for(__u32 i=0; i < (__u32)(block_size / sizeof(__le32)); i++){
		int ret = read_direct_block(img, single_inderect_block_buf[i]);
		if(ret < 0){
			return ret;
		}
	}
	return 0;
}

int read_double_indirect_block(int img, __le32 block_nr){
	if (block_nr == 0){
		return 0;
	}
	__u32 offset = block_nr * block_size;
	ssize_t len = pread(img, single_inderect_block_buf, block_size, offset);
	if(len == -1){
		return -errno;
	}
	for(__u32 i=0; i < (__u32)(block_size / sizeof(__le32)); i++){
		int ret = read_single_indirect_block(img, double_inderect_block_buf[i]);
		if(ret < 0){
			return ret;
		}
	}
	return 0;
}

int dump_dir(int img, int inode_nr)
{
	struct ext2_super_block  sb;
	ssize_t len = pread(img, &sb, sizeof(sb), SUPERBLOCK_OFFSET);
	if(len == -1){
		return -errno;
	}
	
	block_size = 1024 << sb.s_log_block_size;
	int block_group_nr = (inode_nr - 1) / sb.s_inodes_per_group;

	struct ext2_group_desc gd;
	__u32 offset = block_size * (sb.s_first_data_block + 1) + block_group_nr * sizeof(gd);
	len = pread(img, &gd, sizeof(gd), offset);
	if(len == -1){
		return -errno;
	}

	struct ext2_inode in;
	int inode_in_group = (inode_nr - 1) % sb.s_inodes_per_group;
	offset = block_size * gd.bg_inode_table + inode_in_group * sb.s_inode_size;
	len = pread(img, &in, sizeof(in), offset);
	if(len == -1){
		return -errno;
	}

	block_buf = (char*)malloc(block_size);
	int ret = 0;
	//First 12 bloks direct
	for(int i = 0; i < 12; i++){
		ret = read_direct_block(img, in.i_block[i]);
		if(ret < 0){
			return ret;
		}
	}
	single_inderect_block_buf = (__le32*)malloc(block_size);
	//13th block is single-indirect
	ret = read_single_indirect_block(img, in.i_block[12]);
	if(ret < 0){
		return ret;
	}
	//14th block is single-indirect
	double_inderect_block_buf = (__le32*)malloc(block_size);
	ret = read_double_indirect_block(img, in.i_block[13]);
	if(ret < 0){
		return ret;
	}
	return 0;
}