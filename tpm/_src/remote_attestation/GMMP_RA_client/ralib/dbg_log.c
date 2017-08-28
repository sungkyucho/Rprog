#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "dbg_macros.h"

#define OUT_MARGIN 2000
#define STD_BUF 2048

uint32_t max_size;
const char * log_dir;
char * log_file;
pthread_mutex_t log_mutex;
uint8_t to_file;
uint8_t to_sys;

/**
 * @brief log file rotation 
 *        put the time value at the end of the file name.(suffix)
 */
static void log_rotate() {
    int buf_len; 
    char newfile[STD_BUF+1];
	buf_len = strlen(log_file) + 30;

    struct timeval tv;
    gettimeofday(&tv, NULL);

    char tmbuf[500];
    strftime(tmbuf, 500, "%Y-%m-%d_%H:%M:%S", localtime(&tv.tv_sec));
    snprintf(newfile, buf_len, "%s.%s", log_file, tmbuf);
//	printf("rotate log file:%s->%s\n", log_file, newfile);

    int r = rename(log_file, newfile);
    if (r < 0) {
        perror("error rotating current");
    }
}

/*
 * @brief determines the output method according to log level 
 *        use dbg_printf to call this function 
 * @param log_lv logging level  
 * @param file name that belongs to the calling function
 * @param line number that the calling function located 
 * @param errmsg passed message from calling function
*/
void DebugPrintFunc(int log_lv, const char *file, int line, const char *errmsg, ...){
	char buf[STD_BUF+1];
	char logdata[STD_BUF+1];
	va_list ap;

	va_start(ap, errmsg);
	vsnprintf(buf, STD_BUF, errmsg, ap);
	buf[STD_BUF] = '\0';

	if(dbg_lv == DDBG){
		printf("[%s:%d] %s\n", file, line, buf); 
	}
	else if (log_lv & to_sys) 
		syslog(LOG_CRIT, "[%s:%d] %s\n", file, line, buf); 
	else if(log_lv & to_file) {
		int len, written;
        int fd ;
		off_t filesize;

		len = snprintf(logdata, STD_BUF, "(%lu)[%s:%d] %s", (unsigned long)time(NULL), file, line, buf);

		pthread_mutex_lock(&log_mutex);
		fd = open(log_file, O_CREAT | O_APPEND | O_WRONLY, 0644);
        if (fd < 0) {
			perror("Failed to rotating log.");
			pthread_mutex_unlock(&log_mutex);
			return;
        }

		written = write(fd, logdata, len);
		if(written == len){
			if (logdata[len-1] != '\n') {
				write(fd, "\n", 1);
			}
		}
		else
			perror("Failed to write log.");

        filesize = lseek(fd, 0, SEEK_END);
		close(fd);
		if (filesize >= max_size - OUT_MARGIN) {
			log_rotate();
		}
		pthread_mutex_unlock(&log_mutex);
	}
	va_end(ap);
}

/*
 * @brief logging initialize  
 * @param log_size pass 'log_size' value from configuration 
 * @param log_path pass 'log_path' value from configuration
 * @param log_name log file name   
 * @param tofile flag to write custom log file 
 * @param tosys flag to write in syslog 
*/
int init_log(uint32_t log_size, const char * log_path, const char *log_name, uint8_t tofile, uint8_t tosys){
	int file_len =  strlen(log_path) + strlen(log_name) ;

	to_file = tofile;
	to_sys = tosys; 

	max_size = log_size;
	log_dir = log_path;

	log_file = malloc(file_len + 2);
	snprintf(log_file, file_len + 2, "%s/%s", log_path, log_name);

	pthread_mutex_init(&log_mutex,NULL);
	return 1;
}

int clean_log(){
	free(log_file);
	pthread_mutex_destroy(&log_mutex);
	return 1;
}

