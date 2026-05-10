#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/epoll.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define PORT 8080
#define MAX_EVENTS 10

typedef struct{
    int fd;
    int id;
    struct sockaddr_in addr;
    int is_active;
}ClientInfo;

ClientInfo clients[MAX_CLIENTS];
int total_connections = 0;

void broadcast_msg(char *msg, int sender_fd){
    for(int i = 0 ; i < MAX_CLIENTS ; i++){
        if(clients[i].is_active && clients[i].fd != sender_fd){
            if(send(clients[i].fd, msg, strlen(msg), 0) < 0){
                perror("廣播訊息失敗.");
            }
        }
    }
}

int main(void){
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    for(int i = 0 ; i < MAX_CLIENTS ; i++){
        clients[i].is_active = 0;
    }

    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        perror("Server socket 建立失敗");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("Bind 失敗.");
        return 1;
    }

    if(listen(server_fd, 20) < 0){
        perror("Listen 失敗.");
        return 1;
    }
    printf("Server is running.\nListening for incoming connections on Port %d\n", PORT);

    int epoll_fd = epoll_create1(0);
    if(epoll_fd < 0){
        perror("epoll_create 失敗.");
        return 1;
    }

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;

    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0){
        perror("epoll_ctl: server_fd 失敗.");
        return 1;
    }

    while(1){
        int num_of_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if(num_of_events < 0){
            perror("epoll_wait 錯誤.");
            return 1;
        }

        for(int i = 0 ; i < num_of_events ; i++){
            //I: server_fd 發生事件 => 新 client進入
            if(events[i].data.fd == server_fd){
                client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
                if(client_fd < 0){
                    perror("Accept 失敗.");
                    continue;
                }
                printf("Connection established with a client.\n");

                int added = 0;
                for(int i = 0 ; i < MAX_CLIENTS ; i++){
                    if(clients[i].is_active == 0){
                        total_connections++;
                        clients[i].fd = client_fd;
                        clients[i].addr = client_addr;
                        clients[i].is_active = 1;
                        clients[i].id = total_connections;
                        added = 1;
                        break;
                    }
                }

                if(!added){
                    printf("Clients 已達到上限: %d人\n", MAX_CLIENTS);
                    close(client_fd);
                    continue;
                }

                ev.events = EPOLLIN;
                ev.data.fd = client_fd;
                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) < 0){
                    perror("epoll_ctl: client_fd 失敗.");
                    close(client_fd);
                }
            }else{
                //II: client_fd 發生事件 => client 傳送訊息
                int curr_client_fd = events[i].data.fd;
                char buffer[BUFFER_SIZE];
                memset(buffer, 0, BUFFER_SIZE);

                int read_size = recv(curr_client_fd, buffer, BUFFER_SIZE - 1, 0);
                int client_id = -1;

                for(int j = 0 ; j < MAX_CLIENTS ; j++){
                    if(clients[j].is_active && clients[j].fd == curr_client_fd){
                        client_id = clients[j].id;
                        break;
                    }
                }

                if(read_size > 0){
                    buffer[read_size] = '\0';
                    char formatted_msg[BUFFER_SIZE + 64];

                    snprintf(formatted_msg, sizeof(formatted_msg), "[Client %d]: %s", client_id, buffer);
                    printf("%s", formatted_msg);
                    fflush(stdout);
                    broadcast_msg(formatted_msg, curr_client_fd);
                }else{
                    if(read_size == 0){
                        printf("Client %d disconnected.\n", client_id);
                    }else{
                        perror("recv() 發生錯誤.");
                    }
                }

                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, curr_client_fd, NULL);
                close(curr_client_fd);

                for(int j = 0 ; j < MAX_CLIENTS ; j++){
                    if(clients[j].is_active && clients[j].fd == curr_client_fd){
                        clients[j].is_active = 0;
                        break;
                    }
                }
            }
        }
    }

    close(server_fd);
    return 0;
}























