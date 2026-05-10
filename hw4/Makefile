CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -pthread -luring

TARGETS = client server_thread server_epoll
all: $(TARGETS)

client: client.c
		$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

server_thread: server_thread.c
		$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

server_epoll: server_epoll.c
		$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)


clean:
		rm -f $(TARGETS)

.PHONY: all clean
