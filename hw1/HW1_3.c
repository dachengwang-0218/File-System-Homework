#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/time.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include<time.h>

#define FILE_SIZE (100 * 1024 * 1024) //100MB
#define BLOCK_SIZE 4096
#define WRITE_BLOCK_SIZE 2048
#define RAND_OP_TIMES 50000
#define FILENAME "100(3)MB.txt"

void create_file();
void sequential_read();
void sequential_write();
void random_read();
void random_buffered_write();
void random_synchronous_write();

struct timeval start, end;
unsigned long diff;

int main(void) {
    srand(time(NULL));

    create_file();

    printf("--- Standard I/O Benchmark (Homework 1_3)--- \n");

    //sequential_read
    gettimeofday(&start, NULL);
    sequential_read();
    gettimeofday(&end, NULL);
    diff = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
    printf("1. Sequential Read: \t\t\t%f sec\n", diff / 1000000.0);

    //sequential_write
    gettimeofday(&start, NULL);
    sequential_write();
    gettimeofday(&end, NULL);
    diff = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
    printf("2. Sequential Write: \t\t\t%f sec\n", diff / 1000000.0);

    //random read
    gettimeofday(&start, NULL);
    random_read();
    gettimeofday(&end, NULL);
    diff = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
    printf("3. Random Read: \t\t\t%f sec\n", diff / 1000000.0);

    //random buffered write
    gettimeofday(&start, NULL);
    random_buffered_write();
    gettimeofday(&end, NULL);
    diff = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
    printf("4. Random Buffered Write: \t\t%f sec\n", diff / 1000000.0);

    //random synchronous write
    gettimeofday(&start, NULL);
    random_synchronous_write();
    gettimeofday(&end, NULL);
    diff = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
    printf("5. Random Synchronous Write: \t\t%f sec\n", diff / 1000000.0);

    return 0;
}

void create_file() {
    FILE *fp = fopen(FILENAME, "w");
    if(!fp) {
        perror("Failed to create file");
        exit(1);
    }

    char buffer[BLOCK_SIZE];
    for(int i = 0 ; i < BLOCK_SIZE ; i++){
        buffer[i] = '1';
    }

    int num_of_block = FILE_SIZE / BLOCK_SIZE;
    for(int i = 0 ; i < num_of_block ; i++){
        fwrite(buffer, sizeof(char), BLOCK_SIZE, fp);
    }

    fclose(fp);
}

void sequential_read() {
    int fd = open(FILENAME, O_RDONLY);
    if(fd == -1) {
        perror("Failed to open file");
        exit(1);
    }
    
    char *map_ptr = mmap(NULL, FILE_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if(map_ptr == MAP_FAILED){
        perror("Failed to map file");
        exit(1);
    }

    int num_of_block = FILE_SIZE / BLOCK_SIZE;
    char buffer[BLOCK_SIZE];
    for(int i = 0 ; i < num_of_block ; i++){
        long offset = i * BLOCK_SIZE;
        memcpy(buffer, map_ptr + offset, BLOCK_SIZE);
    }

    munmap(map_ptr, FILE_SIZE);
    close(fd);
}

void sequential_write() {
    int fd = open(FILENAME, O_RDWR);
    if(fd == -1) {
        perror("Failed to open file");
        exit(1);
    }

    char *map_ptr = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(map_ptr == MAP_FAILED){
        perror("Failed to map file");
        exit(1);
    }

    int num_of_block = FILE_SIZE / WRITE_BLOCK_SIZE;
    char buffer[WRITE_BLOCK_SIZE];
    for(int i = 0 ; i < WRITE_BLOCK_SIZE ; i++){
        buffer[i] = '2';
    }

    for(int i = 0 ; i < num_of_block ; i++){
        long offset = i * WRITE_BLOCK_SIZE;
        memcpy(map_ptr + offset, buffer, WRITE_BLOCK_SIZE);
    }

    fsync(fd);

    munmap(map_ptr, FILE_SIZE);
    close(fd);
}

void random_read() {
    int fd = open(FILENAME, O_RDONLY);
    if(fd == -1) {
        perror("Failed to open file");
        exit(1);
    }

    char *map_ptr = mmap(NULL, FILE_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if(map_ptr == MAP_FAILED){
        perror("Failed to map file");
        exit(1);
    }

    int num_of_block = FILE_SIZE / BLOCK_SIZE;
    char buffer[BLOCK_SIZE];

    for(int i = 0 ; i < 50000 ; i++){
        long offset = (rand() % num_of_block) * BLOCK_SIZE;
        //lseek(fd, offset, SEEK_SET);
        memcpy(buffer, map_ptr + offset, BLOCK_SIZE);
    }

    munmap(map_ptr, FILE_SIZE);
    close(fd);
}

void random_buffered_write() {
    int fd = open(FILENAME, O_RDWR);
    if(fd == -1) {
        perror("Failed to open file");
        exit(1);
    }

    char *map_ptr = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(map_ptr == MAP_FAILED){
        perror("Failed to map file");
        exit(1);
    }

    int num_of_block = FILE_SIZE / BLOCK_SIZE;
    char buffer[WRITE_BLOCK_SIZE];
    for(int i = 0 ; i < WRITE_BLOCK_SIZE ; i++){
        buffer[i] = '4';
    }

    for(int i = 0 ; i < 50000 ; i++){
        long offset = (rand() % num_of_block) * BLOCK_SIZE;
        //lseek(fd, offset, SEEK_SET);
        memcpy(map_ptr + offset, buffer, WRITE_BLOCK_SIZE);
    }

    fsync(fd);
    munmap(map_ptr, FILE_SIZE);
    close(fd);
}

void random_synchronous_write() {
    int fd = open(FILENAME, O_RDWR);
    if(fd == -1) {
        perror("Failed to open file");
        exit(1);
    }

    char *map_ptr = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(map_ptr == MAP_FAILED){
        perror("Failed to map file");
        exit(1);
    }

    int num_of_block = FILE_SIZE / BLOCK_SIZE;
    char buffer[WRITE_BLOCK_SIZE];
    for(int i = 0 ; i < WRITE_BLOCK_SIZE ; i++){
        buffer[i] = '4';
    }

    for(int i = 0 ; i < 50000 ; i++){
        long offset = (rand() % num_of_block) * BLOCK_SIZE;
        //lseek(fd, offset, SEEK_SET);
        memcpy(map_ptr + offset, buffer, WRITE_BLOCK_SIZE);
        fsync(fd);
    }

    
    munmap(map_ptr, FILE_SIZE);
    close(fd);
}












