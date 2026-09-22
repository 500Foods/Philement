# Makefile for Firebird Brotli Decompression UDR
# Usage:
#   make                # builds UDR shared library
#   sudo make install   # installs to Firebird UDR plugin directory
#   make test           # run tests (requires Firebird SuperServer + isql-fb)
#   make clean
#
# This UDR uses the Firebird OO API (UDR engine) and must be compiled as C++.
# Requires: firebird-devel (provides Firebird headers + libfbclient), libbrotli-dev

# Firebird configuration - try to detect firebird-config
FIREBIRD_CONFIG ?= $(shell command -v firebird-config 2>/dev/null || echo "")

ifneq ($(FIREBIRD_CONFIG),)
    # Use firebird-config for reliable paths
    FIREBIRD_INCLUDE := $(shell $(FIREBIRD_CONFIG) --cflags 2>/dev/null || echo -I/usr/include/firebird)
    FIREBIRD_LIBS := $(shell $(FIREBIRD_CONFIG) --libs 2>/dev/null || echo -lfirebird)
else
    # Fallback: common paths on Fedora
    FIREBIRD_INCLUDE := -I/usr/include/firebird
    FIREBIRD_LIBS := -lfirebird
endif

CC      := g++
CFLAGS  := -fpic -O2 -Wall -Wextra $(FIREBIRD_INCLUDE)
LDFLAGS := -shared
LIBS    := $(FIREBIRD_LIBS) -lbrotlidec
SO_FILE := brotli_decfn.so
SRC     := brotli_decompress.cpp
SO_TEST := brotli_decfn_test

# Firebird UDR plugin directory (default on Linux)
# Can be overridden with UDR_DIR=/custom/path
UDR_DIR ?= /plugins/udr

.PHONY: all install test clean help

all: $(SO_FILE)

$(SO_FILE): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(LIBS)

install: $(SO_FILE)
	@echo "Installing UDR to Firebird UDR plugin directory..."
	@echo "Target: $(UDR_DIR)/$(SO_FILE)"
	sudo mkdir -p "$(UDR_DIR)"
	sudo cp -f "$(SO_FILE)" "$(UDR_DIR)/"
	sudo chmod 755 "$(UDR_DIR)/$(SO_FILE)"
	@echo ""
	@echo "Installation complete."
	@echo "Register the function in SQL with:"
	@echo "  CREATE FUNCTION BROTLI_DECOMPRESS(compressed BLOB)"
	@echo "  RETURNS BLOB"
	@echo "  EXTERNAL NAME 'brotli_decfn!brotli_decompress'"
	@echo "  ENGINE UDR;"
	@echo ""
	@echo "Or via the macro: \$${BROTLI_DECOMPRESS_FUNCTION} from database_firebird.lua"

test: all
	@echo "Testing Brotli UDR decompression..."
	@echo "Note: This test requires a Firebird 4 SuperServer instance."
	@echo "Use the test SQL in test_brotli.sql with isql-fb."

clean:
	rm -f $(SO_FILE) *.o

help:
	@echo "Firebird Brotli Decompression UDR Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build the shared object (brotli_decfn.so)"
	@echo "  install - Install to Firebird UDR plugin directory ($(UDR_DIR))"
	@echo "  test    - Show test instructions"
	@echo "  clean   - Remove build artifacts"
	@echo ""
	@echo "Environment Variables:"
	@echo "  UDR_DIR         - Override UDR plugin directory (default: /plugins/udr)"
	@echo "  FIREBIRD_CONFIG - Path to firebird-config utility (auto-detected)"
