# Makefile for SQLite Brotli Decompression Extension
# Usage:
#   make                                    # builds loadable extension
#   sudo make install                       # installs to system location
#   make test                               # run tests
#   make clean

CC      := gcc
CFLAGS  := -fpic -O2 -Wall -Wextra
LDFLAGS := -shared
LIBS    := -lbrotlidec
SO_FILE := brotli_decompress.so
SRC     := brotli_decompress.c

# Installation directory (common SQLite extension location)
INSTALL_DIR ?= /usr/local/lib

.PHONY: all install test clean

all: $(SO_FILE)

$(SO_FILE): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(LIBS)

install: $(SO_FILE)
	@echo "Installing to $(INSTALL_DIR)..."
	sudo cp -f "$(SO_FILE)" "$(INSTALL_DIR)/"
	sudo chmod 755 "$(INSTALL_DIR)/$(SO_FILE)"
	@echo "Installed successfully."
	@echo ""
	@echo "Load in SQLite with:"
	@echo "  SELECT load_extension('$(INSTALL_DIR)/brotli_decompress');"
	@echo "Or:"
	@echo "  SELECT load_extension('brotli_decompress');"

test:
	@echo "Testing Brotli decompression extension..."
	@echo "Run in SQLite:"
	@echo "  .load ./brotli_decompress"
	@echo "  SELECT 'Extension loaded' AS status;"
	@echo ""
	@echo "Note: Requires actual compressed data for full test"

clean:
	rm -f $(SO_FILE) *.o

.PHONY: help
help:
	@echo "SQLite Brotli Decompression Extension Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build the shared object"
	@echo "  install  - Install to system location"
	@echo "  test     - Show test commands"
	@echo "  clean    - Remove build artifacts"
	@echo ""
	@echo "Environment Variables:"
	@echo "  INSTALL_DIR - Installation directory (default: /usr/local/lib)"