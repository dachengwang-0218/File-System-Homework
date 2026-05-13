#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<liburing.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define PORT 8080
#define ENTRIES 1024

typedef struct{
    int fd;
    int id;
    struct sockaddr_in addr;
    int is_active;
}ClientInfo;

ClientInfo clients[MAX_CLIENTS];
int total_connections = 0;

typedef enum{
    EVENT_ACCEPT,
    EVENT_RECV
}EventType;

typedef struct{
    EventType type;
    int fd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
}IoRequest;

void broadcast_msg(char *msg, int sender_fd){
    for(int i = 0 ; i < MAX_CLIENTS ; i++){
        if(clients[i].is_active && clients[i].fd != sender_fd){
            if(send(clients[i].fd, msg, strlen(msg), 0) < 0){
                perror("廣播訊息失敗.");
            }
        }
    }
}

void add_accept_request(struct io_uring *ring, int server_fd){
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    IoRequest *req = malloc(sizeof(IoRequest));
    req->type = EVENT_ACCEPT;
    req->fd = server_fd;
    req->client_addr_len = sizeof(req->client_addr);

    io_uring_prep_accept(sqe, server_fd, (struct sockaddr *)&req->client_addr, &req->client_addr_len, 0);
    io_uring_sqe_set_data(sqe, req);
    io_uring_submit(ring);
}

void add_recv_request(struct io_uring *ring, int client_fd, IoRequest *req){
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    
    if(req == NULL){
        req = malloc(sizeof(IoRequest));
        req->type = EVENT_RECV;
        req->fd = client_fd;
    }

    memset(req->buffer, 0, BUFFER_SIZE);
    io_uring_prep_recv(sqe, client_fd, req->buffer, BUFFER_SIZE - 1, 0);
    io_uring_sqe_set_data(sqe, req);
    io_uring_submit(ring);

}

int main(void){
    int server_fd;
    struct sockaddr_in server_addr;

    for(int i = 0 ; i < MAX_CLIENTS ; i++){
        clients[i].is_active = 0;
    }

    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        perror("Server socket 建立失敗.");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
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
    printf("Server is running.\nListening for incoming connections on PORT %d.\n", PORT);

    struct io_uring ring;
    if(io_uring_queue_init(ENTRIES, &ring, 0) < 0){
        perror("io_uring init 失敗.");
        return 1;
    }
    
    add_accept_request(&ring, server_fd);

    struct io_uring_cqe *cqe;
    while(1){
        if(io_uring_wait_cqe(&ring, &cqe) < 0){
            perror("io_uring_wait 錯誤.");
            break;
        }

        IoRequest *req = (IoRequest *)io_uring_cqe_get_data(cqe);
        int result = cqe->res;
        io_uring_cqe_seen(&ring, cqe);

        if(req->type == EVENT_ACCEPT){
            int client_fd = result;
            if(client_fd >= 0){
                printf("Connection established with a client.\n");
                int added = 0;
                for(int i = 0 ; i < MAX_CLIENTS ; i++){
                    if(clients[i].is_active == 0){
                        total_connections++;
                        clients[i].fd = client_fd;
                        clients[i].addr = req->client_addr;
                        clients[i].is_active = 1;
                        clients[i].id = total_connections;
                        added = 1;
                        break;
                    }
                }

                if(!added){
                    printf("Clients 已達到上限: %d人\n", MAX_CLIENTS);
                    close(client_fd);
                }else{
                    add_recv_request(&ring, client_fd, NULL);
                }
            }else{
                perror("Accept 失敗.");
            }

            add_accept_request(&ring, server_fd);
            free(req);
        }else if(req->type == EVENT_RECV){
            int read_size = result;
            int client_fd = req->fd;
            int client_id = -1;

            for(int i = 0 ; i < MAX_CLIENTS ; i++){
                if(clients[i].is_active && clients[i].fd == client_fd){
                    client_id = clients[i].id;
                    break;
                }
            }

            if(read_size > 0){
                req->buffer[read_size] = '\0';
                char formatted_msg[BUFFER_SIZE + 64];

                snprintf(formatted_msg, sizeof(formatted_msg), "[Client %d]: %s", client_id, req->buffer);
                printf("%s", formatted_msg);
                fflush(stdout);

                broadcast_msg(formatted_msg, client_fd);
                add_recv_request(&ring, client_fd, req);                
            }else{
                if(read_size == 0){
                    printf("Client %d disconnected.\n");
                }else{
                    perror("recv 錯誤.");
                }

                close(client_fd);
                for(int j = 0 ; j < MAX_CLIENTS ; j++){
                    if(clients[j].is_active && clients[j].fd == client_fd){
                        clients[j].is_active = 0;
                        break;
                    }
                }

                free(req);
            }
        }
    }
    
    io_uring_queue_exit(&ring);
    close(server_fd);
    return 0;
}




























