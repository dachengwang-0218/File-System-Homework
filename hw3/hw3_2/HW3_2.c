#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<liburing.h>
#include<sys/time.h>
#include<sys/stat.h>
#include<sys/types.h>

#define SIZE 65536
#define NUM 200

void multiplication(){
    int A[NUM][NUM], B[NUM][NUM], C[NUM][NUM];
    for(int i = 0 ; i < NUM ; i++){
        for(int j = 0 ; j < NUM ; j++){
            C[i][j] = 0;
            for(int k = 0 ; k < NUM ; k++){
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

double get_time_diff(struct timeval start, struct timeval end){
    return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
}

void blocking_io(const char *src, const char * dest){
    int src_fd, dest_fd;
    ssize_t read_bytes, write_bytes;
    char buffer[SIZE];

    src_fd = open(src, O_RDONLY);
    if(src_fd == -1){
        perror("can't open source file.");
        return;
    }

    dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(dest_fd == -1){
        perror("can't open destination file.");
        return;
    }

    while((read_bytes = read(src_fd, buffer, SIZE)) > 0){
        multiplication();
        write_bytes = write(dest_fd, buffer, read_bytes);
        if(write_bytes == -1){
            break;
        }
    }

    close(src_fd);
    close(dest_fd);
}

void async_io(const char *src, const char *dest, int ENTRY){
    struct io_uring ring;
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;
    struct stat st;
    int src_fd, dest_fd;
    size_t file_size;
    char buffer[SIZE];

    io_uring_queue_init(ENTRY, &ring, 0);

    src_fd = open(src, O_RDONLY);
    if(src_fd == -1){
        perror("can't open source file.");
        return;
    }

    dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(dest_fd == -1){
        perror("can't open destination file.");
        close(src_fd);
        return;
    }

    if (fstat(src_fd, &st) < 0) {
        perror("fstat failed");
        close(src_fd);
        close(dest_fd);
        return;
    }
    file_size = st.st_size;

    off_t read_offset = 0, write_offset = 0;
    int uncom_request = 0;

    while(read_offset < file_size || uncom_request > 0){
        while(uncom_request < ENTRY && read_offset < file_size){
            sqe = io_uring_get_sqe(&ring);
            if(!sqe) break;

            size_t bytes_to_read = SIZE;
            if (read_offset + SIZE > file_size) {
                bytes_to_read = file_size - read_offset;
            }

            io_uring_prep_read(sqe, src_fd, buffer, bytes_to_read, read_offset);

            read_offset += bytes_to_read;
            uncom_request++;
        }

        io_uring_submit(&ring);
        multiplication();

        if(uncom_request > 0){
            int ret = io_uring_wait_cqe(&ring, &cqe);
            if(ret < 0) break;

            int bytes_read = cqe->res;
            io_uring_cqe_seen(&ring, cqe);
            uncom_request--;

            if(bytes_read > 0){
                sqe = io_uring_get_sqe(&ring);
                io_uring_prep_write(sqe, dest_fd, buffer, bytes_read, write_offset);
                write_offset += bytes_read;

                io_uring_submit(&ring);
                io_uring_wait_cqe(&ring, &cqe);
                io_uring_cqe_seen(&ring, cqe);
            }

        }
    }
    close(src_fd);
    close(dest_fd);
    io_uring_queue_exit(&ring);
}

int main(int argc, char *argv[]){
    struct timeval start, end;
    const char *input_file = argv[1];
    const char *output_file = "output_file.txt";
    int ENTRY = atoi(argv[2]);

    system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'");

    gettimeofday(&start, NULL);
    blocking_io(input_file, output_file);
    gettimeofday(&end, NULL);
    printf("Blocking I/O Time: %f seconds\n\n", get_time_diff(start, end));

    system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'");

    gettimeofday(&start, NULL);
    async_io(input_file, output_file, ENTRY);
    gettimeofday(&end, NULL);
    printf("Asynchronous Time: %f seconds\n\n", get_time_diff(start, end));

    return 0;
}














