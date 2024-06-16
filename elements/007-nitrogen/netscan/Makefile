# Makefile for netscan

# Program name
TARGET := netscan

# Compiler
CC := gcc

# C standard
STD := c17

# Compiler flags
CFLAGS := -Wall -Wextra -Wpedantic -std=$(STD) -g
CFLAGS += $(shell pkg-config --cflags glib-2.0 libnm)

# Linker flags
LDFLAGS := $(shell pkg-config --libs glib-2.0 libnm)
LDFLAGS += -lpthread

# Source files
SRCS := netscan.c

# Object files
OBJS := $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Rule to compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# Rule to run the program
run: $(TARGET)
	./$(TARGET)

# Rule to clean up generated files
clean:
	rm -f $(OBJS) $(TARGET)

# Rule to rebuild the project from scratch
rebuild: clean all

# Phony targets
.PHONY: all clean rebuild run
