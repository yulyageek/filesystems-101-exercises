#include <solution.h>
#include <copy_file.h>
#include <scan_dir.h>
#include <ext2.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <linux/types.h>

int dump_file(int img, const char *path, int out)
{
	if (path[0] != '/'){
		return -ENOTDIR;
	}
	int ret = 0;
	char canon_entry_name[MAX_LEN];
	__u8 entry_type;
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
		ret = scan_dir(img, inode_nr, canon_entry_name, entry_type);
		if (ret <= 0){
			return ret;
		}
		inode_nr = ret;
		if(end == NULL){
			ret = copy_file(img, inode_nr, out);
			if(ret < 0){
				return ret;
			}
			break;
		}
		begin = end + 1;
	}
	return 0;
}