CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -pthread -I/usr/local/include
LDFLAGS = -L/usr/local/lib -ljansson

SRCS = klippertest.c klipperconn.c queue.c
OBJS = $(SRCS:.c=.o)
TARGET = klippertest

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)
