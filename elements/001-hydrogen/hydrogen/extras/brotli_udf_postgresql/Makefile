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
	psql -c "CREATE EXTENSION IF NOT EXISTS brotli_decompress;"
	psql -c "SELECT 'Extension installed' AS status;"
	@echo "Testing short string decompression..."
	psql -c "SELECT brotli_decompress(decode('jwWASGVsbG8gV29ybGQhAw==', 'base64')) AS decompressed_short;"
	@echo "Testing medium string decompression..."
	psql -c "SELECT substring(brotli_decompress(decode('H5kDAJwHto06/UBoOKC9hWBQx4wo6LThWqckQtn2Yq2aHkxjyK1mqoQyTpC9+j+X0xvb7uj0wA5wKK3G1DGX//+bgL0oLIIoijJtb2izcv7I1rZUzxGvMQ8rdgVmx7BpQOJtzBRorrsbWW119EN2tiodg2uoDtS/zPE+/EjO1FsWEzDPOMMNoWP7qtF7jnaV314VUnoNUw4zHuXgrFvsKRFeflByH8rsox08e6oZbErSH99qUCBT/fkQ5fdo9XRvnVHmyOLnGPNoeiKZan2+06WjrcLfGVNLs7DuFOdFG345ohqAA/Ob2/6l4vcL8QSo2zDP42yZ0TeHvcYHxNoWRtavWKzINZ4NfF0tVwPm8ykB/JHTYBwjBWVBgOTAxFfrovXNzq7Xsd/hnupf6qE21GsqC/4FRlVMWjZDbccA7Qy2/7hH2VhB2+JKhloHK+GbksQNkIeuuY2iMEo=', 'base64')) from 1 for 4) AS decompressed_medium_prefix;"
	@echo "Testing large string decompression..."
	psql -c "SELECT substring(brotli_decompress(decode('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', 'base64')) from 1 for 4) AS decompressed_large_prefix;"
	@echo "Brotli decompression tests completed successfully"

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