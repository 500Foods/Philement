CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -I./libs/lvgl -I./src -g
LDFLAGS = -lm -lpthread -lGL -lSDL2

SRC_DIR = src
LIB_DIR = libs
BUILD_DIR = build

# Exclude sdltest.c from SOURCES
SOURCES = $(filter-out $(SRC_DIR)/sdltest.c, $(wildcard $(SRC_DIR)/*.c))
LVGL_SOURCES = $(shell find $(LIB_DIR)/lvgl/src -name '*.c')

OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))
LVGL_OBJECTS = $(patsubst $(LIB_DIR)/%.c,$(BUILD_DIR)/%.o,$(LVGL_SOURCES))

TARGET = lvgldemo
SDL_TEST = sdltest

.PHONY: all clean

all: $(TARGET) $(SDL_TEST)

$(TARGET): $(OBJECTS) $(LVGL_OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)

$(SDL_TEST): $(BUILD_DIR)/sdltest.o
	$(CC) $< -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(LIB_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(SDL_TEST)
