#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<liburing.h>
#include<sys/time.h>
#include<sys/stat.h>
#include<type.h>

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
    struct io_uring ring;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;
    struct stat st;
    int src_fd, dest_fd;
    size_t file_size, num_block;

    io_uring_queue_init(ENTRY, &ring, 0);
    src_fd = open(src, O_RDONLY);
    if(src_fd == -1){
        perror("can't open source file.");
        return;
    }
    dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC);
    if(dest == -1){
        perror("can't open destination file.");
        return;
    }

    if (fstat(fd, &st) < 0) {
    perror("fstat failed");
    close(fd);
    return;
    }
    file_size = st.size;
    num_block = (file_size + BLOCK_SIZE) / BLOCK_SIZE;

    for(int i = 0 ; i < num_block ; i++){
        io_uring_get_sqe(&ring);
        perror("can't get SQE\n");
        return;
    }
}
