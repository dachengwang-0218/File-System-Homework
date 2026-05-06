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

int main(int argc, char *argv[]){
    struct timeval start, end;
    const char *input_file = argv[1];
    const char *output_file = "output_file.txt";
    int ENTRY = atoi(argv[2]);

    gettimeofday(&start, NULL);
    blocking_io(input_file, output_file);
    gettimeofday(&end, NULL);
    printf("Blocking I/O Time: %f seconds\n\n", get_time_diff(start, end));

    return 0;
}





















