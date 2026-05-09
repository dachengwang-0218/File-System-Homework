#define MAX_CLIENT 100
#define BUFFER_SIZE 1024
#define SERVER_PORT 8080

typedef struct{
    int fd;
    char ip[16];
    int port;
    int is_active;
}ClientInfo;

ClientInfo clients[MAX_CLIENT];
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
