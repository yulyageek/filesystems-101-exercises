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

char* path_to_proc = "/proc";

void ps(void)
{
	DIR* proc_fd = opendir(path_to_proc);
	if (proc_fd == NULL){
		report_error(path_to_proc, (int)errno);
		return;
	}
	struct dirent* dp;
    	do {
		errno = 0;
		dp = readdir(proc_fd);
		if (dp == NULL && errno != 0){
			//report_error(); Какая ошибка, если ничего не прочиталось 
		}
        	if (dp != NULL) {
			//check that it is PID
			pid_t pid = 0;
			u_int8_t is_pid = 1;
			for (size_t i = 0; i < strlen(dp->d_name); i++){
				if ( (dp->d_name)[i] >= '0' && (dp->d_name)[i] <= '9'){
					pid = pid * 10 + (dp->d_name[i] - '0');
					continue;
				}
				is_pid = 0;
				break; 
			};
			if (is_pid){
				char path[PATH_MAX];
				char exec[PATH_MAX];
				char buff;
				char** envp;
				char** argv;

				sprintf(path, "%s/%s/exe", path_to_proc, dp->d_name);
				ssize_t len = readlink(path, exec, PATH_MAX);
				if(len == -1){
					report_error(path, errno);
					continue;
				}

				sprintf(path, "%s/%s/environ", path_to_proc, dp->d_name);
				int env_fd = open(path, O_RDONLY);
				if(env_fd == -1){
					report_error(path, errno);
					continue;
				}
				int count_envp = 1;
				int len_envp = 0;
				envp = (char**) malloc (1 * sizeof(char*));
				envp[0] = (char*) malloc (10000);
				while ( (len = read(env_fd, &buff, 1)) != 0){
					if(len == -1){
						report_error(path, errno);
						close(env_fd);
						continue;
					}
					envp[count_envp-1][len_envp] = buff;
					len_envp += 1;
					if(buff == '\0'){
						count_envp++;
						len_envp = 0;
						envp = (char**) realloc (envp, count_envp * sizeof(char*));
						envp[count_envp-1] = (char*) malloc (10000);
					}
				}
				envp[count_envp-1] = NULL;
				close(env_fd);
	
				sprintf(path, "%s/%s/cmdline", path_to_proc, dp->d_name);
				int argv_fd = open(path, O_RDONLY);
				if(argv_fd == -1){
					report_error(path, errno);
					continue;
				}
				int count_argv = 1;
				int len_argv = 0;
				argv = (char**) malloc (1 * sizeof(char*));
				argv[0] = (char*) malloc (10000);
				while ( (len = read(argv_fd, &buff, 1)) != 0){
					if(len == -1){
						report_error(path, errno);
						close(argv_fd);
						continue;
					}
					argv[count_argv-1][len_argv] = buff;
					len_argv += 1;
					if(buff == '\0'){
						count_argv++;
						len_argv = 0;
						argv = (char**) realloc (argv, count_argv * sizeof(char*));
						argv[count_argv-1] = (char*) malloc (10000);
					}
				}
				argv[count_argv-1] = NULL;
				close(argv_fd);

				report_process(pid, exec, argv, envp);

				for(int i = 0; i < count_envp; i++){
					free(envp[i]);
				}
				free(envp);
				for(int i = 0; i < count_argv; i++){
					free(argv[i]);
				}
				free(argv);
			}
        	}
    	} while (dp != NULL);
	
	closedir(proc_fd);
}
