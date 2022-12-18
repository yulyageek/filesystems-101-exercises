#include <copy_file.h>
#include <ext2.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>


// static char* block_buf = NULL;
// static __le32* single_inderect_block_buf = NULL;
// static __le32* double_inderect_block_buf = NULL;
static __u32 block_size;
// // static __u32 file_size;
// // static __u32 file_offset = 0;
// // static __u32 write_offset = 0;
// // static __u32 offset;

// __attribute__((destructor)) static void free_all(void){
// 	if (block_buf != NULL)
// 		free(block_buf);
// 	if(single_inderect_block_buf != NULL)
// 		free(single_inderect_block_buf);
// 	if(double_inderect_block_buf != NULL)
// 		free(double_inderect_block_buf);		
// }

int copy_direct_blocks(int img, char*out, int block_nr, off_t *written, off_t *size, off_t *off) {
	if (block_nr == 0){
		return 0;
	} 
	char *buffer = (char*)malloc(block_size);
    int ret = pread(img, buffer, block_size, block_nr * block_size);
    if (ret < 0) {
		free(buffer);
        return -errno;
    }
	if (*off < block_size) {
		size_t read = (*size < block_size - *off) ? *size : block_size - *off;
		memcpy(out + *written, buffer + *off, read);
		*off = 0;
		*size -= read;
		*written += read;
	} else {
		*off -= block_size;
	}
	free(buffer);
    return 0;
}

int copy_single_indirect_block(int img, char*out, int block_nr, off_t *written, off_t *size, off_t *off) {
	if (block_nr == 0){
		return 0;
	} 
	__le32 *buffer = (__le32*)malloc(block_size);
    int ret = pread(img, buffer, block_size, block_nr * block_size);
    if (ret < 0) {
		free(buffer);
        return -errno;
    }
    for (long int i = 0; i < block_size / 4; ++i) {
        ret = copy_direct_blocks(img, out, buffer[i], written,
                                    size, off);
        if (ret < 0) {
			free(buffer);
            return ret;
        }
    }
	free(buffer);
    return 0;
}

int copy_double_indirect_block(int img, char*out, int block_nr, off_t *written, off_t *size, off_t *off)  { 
	if (block_nr == 0){
		return 0;
	}                           
	__le32 *buffer = (__le32*)malloc(block_size);
    int ret = pread(img, buffer, block_size, block_nr * block_size);
    if (ret < 0) {
		free(buffer);
        return -errno;
    }
    int *buffer_int = (int*) buffer;
    for (long int i = 0; i < block_size / 4; ++ i) {
        ret = copy_single_indirect_block(img, out, buffer_int[i], written, size, off);
        if (ret < 0) {
			free(buffer);
			return ret;
		}
    }
	free(buffer);
    return 0;
}

int copy_file(int img, int inode_nr, char* out, size_t size, off_t offset) {
    off_t written = 0;
    off_t off = offset;
    off_t size_ = size;
    int ret = 0;
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
    for (size_t i = 0; i < 12; i++) {
        ret = copy_direct_blocks(img, out, in.i_block[i], &written, &size_, &off);
        if (ret < 0){
			return ret;	
		}
	} 
	ret = copy_single_indirect_block(img, out, in.i_block[12], &written, &size_, &off);
	if (ret < 0) {
		return ret;
	}
	ret = copy_double_indirect_block(img, out, in.i_block[13], &written, &size_, &off);
	if (ret < 0) {
		return ret;
	}
    return size;
}