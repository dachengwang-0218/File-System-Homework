#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<liburing.h>
#include<sys/time.h>
#include<sys/stat.h>

#define ENTRY 8
#define BLOCK_SIZE 4096

double get_time_diff(struct timeval start, struct timeval end){
    return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
}

void blocking_io(const char *src, const char * dest){
    int src_fd, dest_fd;
    ssize_t read_bytes, write_bytes;
    char buffer[BLOCK_SIZE];

    src_fd = open(src, O_RDONLY);
    if(src_fd == -1){
        perror("can't open source file.");
        return;
    }

    dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC);
    if(dest_fd == -1){
        perror("can't open destination file.");
        return;
    }

    while((read_bytes = read(src_fd, buffer, BLOCK_SIZE)) > 0){
        write_bytes = write(dest_src, buffer, BLOCK_SIZE);
        if(write_bytes == -1){
            break;
        }
    }

    close(src_fd);
    close(dest_fd);
}

void async_io(const char *src, const char *dest){
    
}
