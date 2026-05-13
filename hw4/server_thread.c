#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<pthread.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define SERVER_PORT 8080
#define PORT 8080

typedef struct{
    int fd;
    int id;
    struct sockaddr_in addr;
    int is_active;
}ClientInfo;

ClientInfo clients[MAX_CLIENTS];
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
int total_connections = 0;

void broadcast_msg(char *msg, int sender_fd){
    pthread_mutex_lock(&client_mutex);

    for(int i = 0 ; i < MAX_CLIENTS ; i++){
        if(clients[i].is_active && clients[i].fd != sender_fd){
            if(send(clients[i].fd, msg, strlen(msg), 0) < 0){
                perror("廣播訊息失敗.");
            }
        }
    }

    pthread_mutex_unlock(&client_mutex);
}

void *client_handler(void *arg){
    int client_fd = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    int read_size;
    int client_id;

    for(int i = 0 ; i < MAX_CLIENTS ; i++){
        if(clients[i].fd == client_fd){
            client_id = clients[i].id;
        }
    }

    while((read_size = recv(client_fd, buffer, BUFFER_SIZE - 1, 0)) > 0){
        buffer[read_size] = '\0';
        char formatted_msg[BUFFER_SIZE + 64];

        snprintf(formatted_msg, sizeof(formatted_msg), "[Client %d]: %s", client_id, buffer);
        printf("%s", formatted_msg);
        fflush(stdout);
        broadcast_msg(formatted_msg, client_fd);

        memset(buffer, 0, BUFFER_SIZE);
    }

    if(read_size == 0){
        printf("Client %d Disconnected.\n", client_id);
    }

    pthread_mutex_lock(&client_mutex);
    for(int i = 0 ; i < MAX_CLIENTS ; i++){
        if(clients[i].is_active && clients[i].fd == client_fd){
            clients[i].is_active = 0;
            break;
        }
    }
    pthread_mutex_unlock(&client_mutex);

    close(client_fd);
    pthread_exit(NULL);
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

    while(1){
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if(client_fd < 0){
            perror("Accept 失敗.");
            return 1;
        }
        printf("Connection established with a client!\n");
        
        pthread_mutex_lock(&client_mutex);
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
        pthread_mutex_unlock(&client_mutex);

        if(!added){
            printf("Clients 已達到上限: %d人\n", MAX_CLIENTS);
            close(client_fd);
            continue;
        }

        int *new_client_fd = malloc(sizeof(int));
        *new_client_fd = client_fd;

        pthread_t thread_id;
        if(pthread_create(&thread_id, NULL, client_handler, (void *)new_client_fd) < 0){
            perror("無法建立 Worker Thread.");
            free(new_client_fd);
            continue;
        }

        pthread_detach(thread_id);
    }
    
    close(server_fd);
    return 0;
}
