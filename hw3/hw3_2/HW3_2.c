#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<liburing.h>
#include<sys/time.h>
#include<sys/stat.h>
#include<sys/types.h>

#define SIZE 65536
#define NUM 20

typedef struct{
    int op_type;
    int buf_idx;
    off_t offset;
} io_info;

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
    struct stat st;
    int src_fd, dest_fd;
    size_t file_size;

    char **buffers = malloc(ENTRY * sizeof(char*));
    for(int i = 0 ; i < ENTRY ; i++){
        buffers[i] = malloc(SIZE);
    }

    io_info *info = malloc(ENTRY * sizeof(io_info));
    int *buffer_in_use = calloc(ENTRY, sizeof(int));

    io_uring_queue_init(ENTRY, &ring, 0);

    src_fd = open(src, O_RDONLY);
    if(src_fd == -1){
        perror("can't open source file.");
        goto cleanup_mem;
    }

    dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(dest_fd == -1){
        perror("can't open destination file.");
        close(src_fd);
        goto cleanup_mem;
    }

    if (fstat(src_fd, &st) < 0) {
        perror("fstat failed");
        close(src_fd);
        close(dest_fd);
        goto cleanup_mem;
    }
    file_size = st.st_size;

    off_t read_offset = 0;
    off_t bytes_written = 0;
    int active_reqs = 0;

    while(bytes_written < file_size || active_reqs > 0){
        int submit_this_round = 0;
        
        for(int i = 0 ; i < ENTRY && read_offset < file_size ; i++){
            if(buffer_in_use[i] == 0){
                struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
                if(!sqe) break;

                size_t bytes_to_read = SIZE;
                if(read_offset + SIZE > file_size){
                    bytes_to_read = file_size - read_offset;
                }

                io_uring_prep_read(sqe, src_fd, buffers[i], bytes_to_read, read_offset);

                info[i].op_type = 0;
                info[i].buf_idx = i;
                info[i].offset = read_offset;
                io_uring_sqe_set_data(sqe, &info[i]);

                read_offset += bytes_to_read;
                buffer_in_use[i] = 1;
                active_reqs++;
                submit_this_round++;
            }
        }

        if(submit_this_round > 0){
            io_uring_submit(&ring);
            multiplication();
        }

        if(active_reqs > 0){
            struct io_uring_cqe *cqe;
            int ret = io_uring_wait_cqe(&ring, &cqe);
            if(ret < 0) break;

            io_info *req = (io_info *)io_uring_cqe_get_data(cqe);
            int res = cqe->res;
            io_uring_cqe_seen(&ring, cqe);

            if(req->op_type == 0){
                if(res > 0){
                    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
                    if(sqe){
                        io_uring_prep_write(sqe, dest_fd, buffers[req->buf_idx], res, req->offset);
                        req->op_type = 1;
                        io_uring_sqe_set_data(sqe, req);
                        io_uring_submit(&ring);
                    }
                }else{
                    buffer_in_use[req->buf_idx] = 0;
                    active_reqs--;
                }
            }else if(req->op_type == 1){
                if(res > 0){
                    bytes_written += res;
                }
                buffer_in_use[req->buf_idx] = 0;
                active_reqs--;
            }
        }
    }

    close(src_fd);
    close(dest_fd);
    io_uring_queue_exit(&ring);

cleanup_mem:
    for(int i= 0 ; i < ENTRY ; i++) free(buffers[i]);
    free(buffers);
    free(info);
    free(buffer_in_use);
}

int main(int argc, char *argv[]){
    struct timeval start, end;
    const char *input_file = argv[1];
    const char *output_file = "output.txt";
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














