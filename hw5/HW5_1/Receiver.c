#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUFFER_SIZE 4096

int main(int argc, char *argv[]){
    int listen_port = atoi(argv[1]);
    int sockfd;
    char buffer[BUFFER_SIZE];

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        perror("socket Fail.");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(listen_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("bind Fail.");
        return 1;
    }
    printf("Receiver is listening on port %d...\n", listen_port);

    while(1){
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&client_addr, &client_len);
        if(bytes < 0){
            perror("recvfrom Fail.");
            close(sockfd);
            break;
        }
        buffer[bytes] = '\0';

        printf("\n----- packet received -----\n");
        printf("Incoming Message:\t%s\n", buffer);
        printf("Source IP:\t%s\n", inet_ntoa(client_addr.sin_addr));
        printf("Source Port:\t%d\n", ntohs(client_addr.sin_port));
    }

    close(sockfd);
    return 0;
}