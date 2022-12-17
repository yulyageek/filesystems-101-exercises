#include <copy_file.h>
#include <ext2.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

static char* block_buf = NULL;
static __le32* single_inderect_block_buf = NULL;
static __le32* double_inderect_block_buf = NULL;
static __u32 block_size;
static __u32 file_size;
static __u32 file_offset = 0;

__attribute__((destructor)) static void free_all(void){
	if (block_buf != NULL)
		free(block_buf);
	if(single_inderect_block_buf != NULL)
		free(single_inderect_block_buf);
	if(double_inderect_block_buf != NULL)
		free(double_inderect_block_buf);		
}

int copy_direct_block(int img, char* out, __le32 block_nr, off_t offset){
	if(block_nr == 0){
		return 0;
	}
	int read_offset = block_size * block_nr;
	ssize_t len = pread(img, block_buf, block_size, read_offset);
	if(len == -1){
		return -errno;
	}
	__u32 write_len = ((__u32)len<file_size-file_offset?(__u32)len:file_size-file_offset);
	if (offset < write_len){
		memcpy(out, block_buf + offset, write_len-offset);
	} else {
		offset -= write_len;
	}
	// memcpy(out, block_buf, ((__u32)len<file_size-file_offset?(__u32)len:file_size-file_offset));
	file_offset += write_len;
	return 0;
}

int copy_single_indirect_block(int img, char* out, __le32 block_nr, off_t offset){
	if (block_nr == 0){
		return 0;
	}

	int read_offset = block_size * block_nr;
	ssize_t len = pread(img, single_inderect_block_buf, block_size, read_offset);
	if(len == -1){
		return -errno;
	}

	for(__u32 i=0; i < (__u32)(block_size / sizeof(__le32)); i++){
		int ret = copy_direct_block(img, out, single_inderect_block_buf[i], offset);
		if(ret < 0){
			return ret;
		}
	}
	return 0;
}

int copy_double_indirect_block(int img, char* out, __le32 block_nr, off_t offset){
	if (block_nr == 0){
		return 0;
	}

	int read_offset = block_size * block_nr;
	ssize_t len = pread(img, double_inderect_block_buf, block_size, read_offset);
	if(len == -1){
		return -errno;
	}

	for(__u32 i=0; i < (__u32)(block_size / sizeof(__le32)); i++){
		int ret = copy_single_indirect_block(img, out, double_inderect_block_buf[i], offset);
		if(ret < 0){
			return ret;
		}
	}
	return 0;
}

int copy_file(int img, int inode_nr, char *out, off_t offset)
{
	struct ext2_super_block  sb;
	ssize_t len = pread(img, &sb, sizeof(sb), SUPERBLOCK_OFFSET);
	if(len == -1){
		return -errno;
	}
	
	block_size = 1024 << sb.s_log_block_size;
	int block_group_nr = (inode_nr - 1) / sb.s_inodes_per_group;

	struct ext2_group_desc gd;
	int read_offset = block_size * (sb.s_first_data_block + 1) + block_group_nr * sizeof(gd);
	len = pread(img, &gd, sizeof(gd), read_offset);
	if(len == -1){
		return -errno;
	}

	struct ext2_inode in;
	int inode_in_group = (inode_nr - 1) % sb.s_inodes_per_group;

	read_offset = block_size * gd.bg_inode_table + inode_in_group * sb.s_inode_size;
	len = pread(img, &in, sizeof(in), read_offset);
	if(len == -1){
		return -errno;
	}

	file_size = in.i_size;
	block_buf = (char*)malloc(block_size);
	int ret = 0;
	//First 12 bloks direct
	for(int i = 0; i < 12; i++){
		ret = copy_direct_block(img, out, in.i_block[i], offset);
		if(ret < 0){
			return ret;
		}
	}
	single_inderect_block_buf = (__le32*)malloc(block_size);
	//13th block is single-indirect
	ret = copy_single_indirect_block(img, out, in.i_block[12], offset);
	if(ret < 0){
		return ret;
	}
	//14th block is single-indirect
	double_inderect_block_buf = (__le32*)malloc(block_size);
	ret = copy_double_indirect_block(img, out, in.i_block[13], offset);
	if(ret < 0){
		return ret;
	}
	return file_size;
}