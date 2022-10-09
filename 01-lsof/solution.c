#include <solution.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/limits.h>

void lsof(void)
{
	char* path_to_proc = "/proc";
	DIR* proc = opendir(path_to_proc);
	if (proc == NULL){
		report_error(path_to_proc, errno);
		return;
	}

	struct dirent* dp;
    	while(1){
		errno = 0;
		dp = readdir(proc);
		if (dp == NULL && errno != 0){
			report_error(path_to_proc, errno);
			break;
		}
        	if (dp == NULL) {
			break;
		}
		//check that it is PID
		u_int8_t is_pid = 1;
		for (size_t i = 0; i < strlen(dp->d_name); i++){
			if ( (dp->d_name)[i] >= '0' && (dp->d_name)[i] <= '9'){
				continue;
			}
			is_pid = 0;
			break; 
		};
		if (!is_pid){
			continue;
		}
		char path_to_fd[PATH_MAX];
		char file[PATH_MAX];
		char path_to_file[PATH_MAX * 2];
		sprintf(path_to_fd, "%s/%s/fd", path_to_proc, dp->d_name);
		struct dirent* fd_dir;
		DIR* proc_fd = opendir(path_to_fd);
		if (proc_fd == NULL){
			report_error(path_to_fd, errno);
			continue;
		}
		while(1){
			errno = 0;
			fd_dir = readdir(proc_fd);
			if (fd_dir == NULL && errno != 0){
				report_error(path_to_fd, errno);
				break;
			}
			if (fd_dir == NULL){
				break;
			}
			//Skip . and ..
			if(strcmp(fd_dir->d_name, ".") == 0 || strcmp(fd_dir->d_name, "..") == 0){
				continue;
			}
			sprintf(path_to_file, "%s/%s", path_to_fd, fd_dir->d_name);
			ssize_t len = readlink(path_to_file, file, PATH_MAX);
			if(len == -1){
				report_error(path_to_file, errno);
				continue;
			}
			file[len] = '\0';
			report_file(file);
		}
		closedir(proc_fd);
	}
	closedir(proc);
}
