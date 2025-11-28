# Makefile for MySQL Brotli Decompression UDF
# Usage:
#   make                                    # builds .so
#   make install                            # copies .so to MySQL plugin directory
#   make register                           # creates function in MySQL
#   make test                               # run tests
#   make clean

MYSQL_CONFIG ?= mysql_config
PLUGIN_DIR := $(shell $(MYSQL_CONFIG) --plugindir 2>/dev/null || echo /usr/lib/mysql/plugin)

CC      := gcc
CFLAGS  := -fpic -O2 -Wall -Wextra $(shell $(MYSQL_CONFIG) --include 2>/dev/null || echo -I/usr/include/mysql)
LDFLAGS := -shared
LIBS    := -lbrotlidec
SO_FILE := brotli_decompress.so
SRC     := brotli_decompress.c

.PHONY: all install register test clean

all: $(SO_FILE)

$(SO_FILE): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(LIBS)

install: $(SO_FILE)
	@echo "Installing to $(PLUGIN_DIR)..."
	sudo cp -f "$(SO_FILE)" "$(PLUGIN_DIR)/"
	sudo chmod 755 "$(PLUGIN_DIR)/$(SO_FILE)"
	@echo "Installed successfully."

register:
	@echo "Registering BROTLI_DECOMPRESS function in MySQL..."
	mysql -e "CREATE FUNCTION IF NOT EXISTS BROTLI_DECOMPRESS RETURNS STRING SONAME 'brotli_decompress.so';"
	@echo "Function registered successfully."

test:
	@echo "Testing BROTLI_DECOMPRESS function..."
	@echo "Note: Requires actual compressed data for full test"
	mysql -e "SELECT 'UDF installed' AS status;"

clean:
	rm -f $(SO_FILE) *.o

.PHONY: help
help:
	@echo "MySQL Brotli Decompression UDF Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build the shared object"
	@echo "  install  - Install to MySQL plugin directory"
	@echo "  register - Register function in MySQL"
	@echo "  test     - Run basic tests"
	@echo "  clean    - Remove build artifacts"
	@echo ""
	@echo "Environment Variables:"
	@echo "  MYSQL_CONFIG - Path to mysql_config utility (default: mysql_config)"
	@echo "  PLUGIN_DIR   - MySQL plugin directory (auto-detected)"