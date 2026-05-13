CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -pthread -luring

TARGETS = client server_thread server_epoll server_io_uring
all: $(TARGETS)

client: client.c
		$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

server_thread: server_thread.c
		$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

server_epoll: server_epoll.c
		$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

server_io_uring: server_io_uring.c
		$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
		rm -f $(TARGETS)

.PHONY: all clean
