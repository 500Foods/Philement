CC = gcc
CFLAGS = -Wall -Wextra -std=c17 -pedantic `pkg-config --cflags libnm glib-2.0`  
LDFLAGS = -lpthread -ljansson `pkg-config --libs libnm glib-2.0` -lm 

SOURCES = hydro.c systeminfo.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = hydro

.PHONY: all debug clean

all: LDFLAGS += $(shell pkg-config --libs jansson)
all: $(EXECUTABLE)

debug: CFLAGS += -g -fsanitize=address
debug: LDFLAGS += -fsanitize=address
debug: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
