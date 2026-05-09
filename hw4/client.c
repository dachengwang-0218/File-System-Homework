#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<pthread.h>

#define BUFFER_SIZE 1024

void *msg_recv_handler(void *socket_descriptor){
    int sock_fd = *(int*)socket_descriptor;
    char msg[BUFFER_SIZE];
    int recv_len = 0;
    
    while(1){
        memset(msg, 0, BUFFER_SIZE);
        recv_len = recv(sock_fd, msg, BUFFER_SIZE, 0);

        if(recv_len > 0){
            printf("%s", msg);
            fflush(stdout);
        }else if(recv_len == 0){
            printf("Servr 已斷線.\n");
            break;
        }else{
            perror("recv() 發生錯誤.");
        }
    }
    close(sock_fd);
    exit(0);
}

int main(int argc, char *argv[]){
    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int sock_fd;
    struct sockaddr_in server_addr;
    char msg[BUFFER_SIZE];
    pthread_t recv_thread;

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_fd == 0){
        perror("無法建立socket.");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if(inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0){
        perror("Invalid address or address not supported.");
        return 1;
    }

    if(connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("Connection Failed.");
        return 1;
    }
    printf("Successfully connect to the server.\n");
        
    if(pthread_create(&recv_thread, NULL, msg_recv_handler, (void*)&sock_fd) < 0){
        perror("無法建立Recv Thread.");
        return 1;
    }

    while(1){
        memset(msg, 0, BUFFER_SIZE);

        if(fgets(msg, BUFFER_SIZE, stdin) != NULL){
            if(strncmp(msg, "exit", 4) == 0){
                printf("中斷連線...\n");
                break;
            }

            if(send(sock_fd, msg, strlen(msg), 0) < 0){
                perror("訊息傳送失敗.");
                break;
            }
        }
    }

    close(sock_fd);
    return 0;
}



















