CC = gcc
CFLAGS = -Wall -g

TARGETS = capture sender receiver modifier

all: $(TARGETS)

capture: HW5_1.c
		$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

sender: Sender.c
		$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

receiver: Receiver.c
		$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

modifier: HW5_1_5.c
		$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
		rm -f $(TARGETS)

.PHONY: all clean