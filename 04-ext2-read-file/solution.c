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
static __u32 block_size;
static __u32 size;
static __u32 offset = 0;

__attribute__((destructor)) void free_all(void){
	if (block_buf != NULL)
		free(block_buf);
	if(single_inderect_block_buf != NULL)
		free(single_inderect_block_buf);
	if(double_inderect_block_buf != NULL)
		free(double_inderect_block_buf);		
}

int copy_direct_block(int img, int out, __le32 block_nr){
	if(block_nr == 0x0000){
		return 0;
	}
	lseek(img, block_size * block_nr, SEEK_SET);
	int len = read(img, block_buf, block_size);
	if(len < (int)block_size){
		return -errno;
	}
	len = write(out, block_buf, ((__u32)len<size-offset?(__u32)len:size-offset));
	if(len < (int)block_size){
		return -errno;
	}
	offset += len;
	return 0;
}

int copy_single_indirect_block(int img, int out, __le32 block_nr){
	if (block_nr == 0x0000){
		return 0;
	}
	lseek(img, block_size * block_nr, SEEK_SET);
	int len = read(img, single_inderect_block_buf, block_size);
	if(len < (int)block_size){
		return -errno;
	}
	for(__u32 i=0; i < (__u32)(block_size / sizeof(__le32)); i++){
		//fprintf(stderr, "%i: %d\n", i, single_inderect_block_buf[i]);
		int ret = copy_direct_block(img, out, single_inderect_block_buf[i]);
		if(ret < 0){
			return ret;
		}
	}
	return 0;
}

int copy_double_indirect_block(int img, int out, __le32 block_nr){
	if (block_nr == 0x0000){
		return 0;
	}
	lseek(img, block_size * block_nr, SEEK_SET);
	int len = read(img, double_inderect_block_buf, block_size);
	if(len < (int)block_size){
		return -errno;
	}
	for(__u32 i=0; i < (__u32)(block_size / sizeof(__le32)); i++){
		int ret = copy_single_indirect_block(img, out, double_inderect_block_buf[i]);
		if(ret < 0){
			return ret;
		}
	}
	return 0;
}

int dump_file(int img, int inode_nr, int out)
{

	struct ext2_super_block  sb;
	lseek(img, 1024, SEEK_SET);
	//int len  = read(img, &sb, sizeof(struct ext2_super_block));
	int len  = read(img, &sb, sizeof(sb));
	if(len < 0){
		return -errno;
	}
	block_size = 1024 << sb.s_log_block_size;

	//fprintf(stderr, "inode %d, per group %d\n", inode_nr, sb.s_inodes_per_group);
	struct ext2_group_desc gd;
	lseek(img, block_size * (sb.s_first_data_block + 1), SEEK_SET);
	//len  = read(img, &gd, sizeof(struct ext2_group_desc));
	len  = read(img, &gd, sizeof(gd));
	if(len < 0){
		return -errno;
	}
	
	//lseek(img, block_size * (gd.bg_inode_table) + (inode_nr - 1) * sizeof(struct ext2_inode), SEEK_SET);
	struct ext2_inode in;
	lseek(img, block_size * (gd.bg_inode_table) + (inode_nr - 1) * sizeof(in), SEEK_SET);
	//len  = read(img, &in, sizeof(struct ext2_inode));
	len  = read(img, &in, sizeof(in));
	if(len < 0){
		return -errno;
	}
	size = in.i_size;
	block_buf = (char*)malloc(block_size);
	int ret = 0;
	//First 12 bloks direct
	for(int i = 0; i < 12; i++){
		fprintf(stderr, "%d: %d\n", i, in.i_block[i]);
		ret = copy_direct_block(img, out, in.i_block[i]);
		if(ret < 0){
			return ret;
		}
	}
	single_inderect_block_buf = (__le32*)malloc(block_size);
	//13th block is single-indirect
	ret = copy_single_indirect_block(img, out, in.i_block[12]);
	if(ret < 0){
		return ret;
	}
	//14th block is single-indirect
	double_inderect_block_buf = (__le32*)malloc(block_size);
	ret = copy_double_indirect_block(img, out, in.i_block[13]);
	if(ret < 0){
		return ret;
	}
	if(offset != size){
		fprintf(stderr, "my_size %d, file_size %d\n", offset, size);
		return -1;
	}
	return 0;
}
