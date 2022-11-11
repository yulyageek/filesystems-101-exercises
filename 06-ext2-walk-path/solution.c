#include <solution.h>
#include <ext2.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <linux/types.h>

char* block_buf = NULL;
__le32* single_inderect_block_buf = NULL;
__le32* double_inderect_block_buf = NULL;
char entry_name[MAX_LEN];
char canon_entry_name[MAX_LEN];
__u8 entry_type;
__u32 block_size;
struct ext2_super_block  sb;
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

int (*process_block)(int, __le32, int);
int (*check_and_return)(int);

int g0 (int x){
	return x > 0;
}

int le0 (int x){
	return x <= 0;
}

int scan_dir(int img, __le32 block_nr, int out){
	(void) out;
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
		if(!strcmp(entry_name, canon_entry_name) && entry.file_type == entry_type){
			return entry.inode;
		}
		if(entry.rec_len + offset == (block_nr+1) * block_size){
			return 0;
		}
		offset += entry.rec_len;
	}
	return 0;
}

int copy_file(int img, __le32 block_nr, int out){
	if(block_nr == 0){
		return 0;
	}
	ssize_t len = pread(img, block_buf, block_size,  block_size * block_nr);
		if(len == -1){
			return -errno;
	}
	len = write(out, block_buf, ((__u32)len<size-offset?(__u32)len:size-offset));
	if(len < (int)block_size){
		return -errno;
	}
	offset += len;
	return 0;
}

int read_single_indirect_block(int img, __le32 block_nr, int out){
	if (block_nr == 0){
		return 0;
	}
	__u32 offset = block_nr * block_size;
	ssize_t len = pread(img, single_inderect_block_buf, block_size, offset);
	if(len == -1){
		return -errno;
	}
	for(__u32 i=0; i < (__u32)(block_size / sizeof(__le32)); i++){
		int ret = process_block(img, single_inderect_block_buf[i], out);
		if(check_and_return(ret)){
			return ret;
		}
	}
	return 0;
}

int read_double_indirect_block(int img, __le32 block_nr, int out){
	if (block_nr == 0){
		return 0;
	}
	__u32 offset = block_nr * block_size;
	ssize_t len = pread(img, single_inderect_block_buf, block_size, offset);
	if(len == -1){
		return -errno;
	}
	for(__u32 i=0; i < (__u32)(block_size / sizeof(__le32)); i++){
		int ret = read_single_indirect_block(img, double_inderect_block_buf[i], out);
		if(check_and_return(ret)){
			return ret;
		}
	}
	return 0;
}

int process_inode(int img, int inode_nr, int out)
{
	int block_group_nr = (inode_nr - 1) / sb.s_inodes_per_group;
	struct ext2_group_desc gd;
	__u32 offset = block_size * (sb.s_first_data_block + 1) + block_group_nr * sizeof(gd);
	int len = pread(img, &gd, sizeof(gd), offset);
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
	if (block_buf == NULL){
		block_buf = (char*)malloc(block_size);
	}
	size = in.i_size;
	int ret = 0;
	//First 12 bloks direct
	for(int i = 0; i < 12; i++){
		ret = process_block(img, in.i_block[i], out);
		if(check_and_return(ret)){
			return ret;
		}
	}
	if (single_inderect_block_buf == NULL){
		single_inderect_block_buf = (__le32*)malloc(block_size);
	}
	//13th block is single-indirect
	ret = read_single_indirect_block(img, in.i_block[12], out);
	if(check_and_return(ret)){
		return ret;
	}
	//14th block is single-indirect
	if (double_inderect_block_buf == NULL){
		double_inderect_block_buf = (__le32*)malloc(block_size);
	}
	ret = read_double_indirect_block(img, in.i_block[13], out);
	if(check_and_return(ret)){
		return ret;
	}
	return -ENOENT;
}

int dump_file(int img, const char *path, int out)
{
	if (path[0] != '/'){
		return -ENOTDIR;
	}

	(void) out;
	ssize_t len = pread(img, &sb, sizeof(sb), SUPERBLOCK_OFFSET);
	if(len == -1){
		return -errno;
	}
	
	block_size = 1024 << sb.s_log_block_size;
	int inode_nr = 2; //root_inode
	char* begin = (char*)path + 1;
	char* end;
	int entry_name_len;
	while(1){
		end = index(begin, '/');
		if(end == NULL){
			entry_type = EXT2_FT_REG_FILE;
			entry_name_len = path+strlen(path)-begin;
		} else{
			entry_type = EXT2_FT_DIR;
			entry_name_len = end-begin;
		} 
		memcpy(canon_entry_name, begin, entry_name_len);
		canon_entry_name[entry_name_len] = '\0';
		//printf("%s/\t%ld\n", canon_entry_name, strlen(canon_entry_name));
		process_block = scan_dir;
		check_and_return = g0;
		inode_nr = process_inode(img, inode_nr, out);
		if (inode_nr <= 0){
			return inode_nr;
		}
		if(end == NULL){
			process_block = copy_file;
			check_and_return = le0;
			inode_nr = process_inode(img, inode_nr, out);
			if(inode_nr < 0){
				return inode_nr;
			}
			break;
		}
		begin = end + 1;
	}
	return 0;
}