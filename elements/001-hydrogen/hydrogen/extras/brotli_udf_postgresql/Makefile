# Makefile for PostgreSQL Brotli Decompression Extension
# Usage:
#   make                # builds extension
#   sudo make install   # installs to PostgreSQL
#   make clean

# PostgreSQL configuration
PG_CONFIG ?= pg_config

# Check if pg_config exists
ifeq ($(shell command -v $(PG_CONFIG) 2>/dev/null),)
    $(error pg_config not found. Install postgresql-server-dev package)
endif

# Get PostgreSQL paths
PG_INCLUDEDIR := $(shell $(PG_CONFIG) --includedir-server 2>/dev/null || echo /usr/include/postgresql/server)
PG_PKGLIBDIR := $(shell $(PG_CONFIG) --pkglibdir 2>/dev/null || echo /usr/lib/postgresql/lib)
PG_SHAREDIR := $(shell $(PG_CONFIG) --sharedir 2>/dev/null || echo /usr/share/postgresql)

# Build configuration
CC := gcc
CFLAGS := -fpic -O2 -Wall -Wextra -I$(PG_INCLUDEDIR)
LDFLAGS := -shared
LIBS := -lbrotlidec
SO_FILE := brotli_decompress.so
SRC := brotli_decompress.c

.PHONY: all install test clean help

all: $(SO_FILE)

$(SO_FILE): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(LIBS)

install: $(SO_FILE) brotli_decompress.control brotli_decompress--1.0.sql
	@echo "Installing extension to PostgreSQL..."
	sudo mkdir -p "$(PG_PKGLIBDIR)"
	sudo cp -f "$(SO_FILE)" "$(PG_PKGLIBDIR)/"
	sudo chmod 755 "$(PG_PKGLIBDIR)/$(SO_FILE)"
	@echo "Installing control and SQL files..."
	sudo mkdir -p "$(PG_SHAREDIR)/extension"
	sudo cp -f brotli_decompress.control "$(PG_SHAREDIR)/extension/"
	sudo cp -f brotli_decompress--1.0.sql "$(PG_SHAREDIR)/extension/"
	@echo "Installation complete. Create extension in PostgreSQL with:"
	@echo "  CREATE EXTENSION brotli_decompress;"

test:
	@echo "Testing Brotli decompression extension..."
	@echo "Run in PostgreSQL:"
	@echo "  CREATE EXTENSION IF NOT EXISTS brotli_decompress;"
	@echo "  SELECT 'Extension installed' AS status;"

clean:
	rm -f $(SO_FILE) *.o

help:
	@echo "PostgreSQL Brotli Decompression Extension Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build the shared object"
	@echo "  install  - Install to PostgreSQL directories"
	@echo "  test     - Show test commands"
	@echo "  clean    - Remove build artifacts"
	@echo ""
	@echo "Detected paths:"
	@echo "  Include:  $(PG_INCLUDEDIR)"
	@echo "  Library:  $(PG_PKGLIBDIR)"
	@echo "  Share:    $(PG_SHAREDIR)"