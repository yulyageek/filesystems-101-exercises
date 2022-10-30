#include <solution.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

int dump_file(int img, int inode_nr, int out)
{
	(void) img;
	(void) inode_nr;
	(void) out;
	
	struct ext2_super_block sb;
	lseek(img, 1024, SEEK_SET);
	//int len  = read(img, &sb, sizeof(struct ext2_super_block));
	int len  = read(img, &sb, sizeof(sb));
	if(len < 0){
		return -errno;
	}
	__u32 block_size = 1024 << sb.s_log_block_size;

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

	char *buf = (char*)malloc(block_size);
	for(int i = 0; i < 15; i++){
		if(!in.i_block[i]){
			break;
		}
		lseek(img, block_size * in.i_block[i], SEEK_SET);
		len = read(img, buf, block_size);
		if(len < (int)block_size){
			free(buf);
			return -errno;
		}
		len = write(out, buf, block_size);
		if(len < (int)block_size){
			free(buf);
			return -errno;
		}
		free(buf);
	}

	return 0;
}
