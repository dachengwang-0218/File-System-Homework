CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -pthread -luring

TARGETS = client server_thread
all: $(TARGETS)

client: client.c
		$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

server_thread: server_thread.c
		$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
		rm -f $(TARGETS)

.PHONY: all clean
