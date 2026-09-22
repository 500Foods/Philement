# Makefile for Firebird JSON Value UDR
# Usage:
#   make                # builds UDR shared library
#   sudo make install   # installs to Firebird UDR plugin directory
#   make test           # run tests (requires Firebird SuperServer + isql-fb)
#   make clean
#
# This UDR uses the Firebird OO API (UDR engine) and must be compiled as C++.
# Requires: firebird-devel, libjansson-devel, gcc-c++

# Firebird configuration - try to detect firebird-config
FIREBIRD_CONFIG ?= $(shell command -v firebird-config 2>/dev/null || echo "")

ifneq ($(FIREBIRD_CONFIG),)
    FIREBIRD_INCLUDE := $(shell $(FIREBIRD_CONFIG) --cflags 2>/dev/null || echo -I/usr/include/firebird)
    FIREBIRD_LIBS := $(shell $(FIREBIRD_CONFIG) --libs 2>/dev/null || echo -lfirebird)
else
    FIREBIRD_INCLUDE := -I/usr/include/firebird
    FIREBIRD_LIBS := -lfirebird
endif

CC      := g++
CFLAGS  := -fpic -O2 -Wall -Wextra $(FIREBIRD_INCLUDE)
LDFLAGS := -shared
LIBS    := $(FIREBIRD_LIBS) -ljansson
SO_FILE := json_udfn.so
SRC     := json_value.cpp

# Firebird UDR plugin directory (default on Linux)
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
	@echo "  CREATE FUNCTION JSON_VALUE(json_doc BLOB SUB_TYPE TEXT, json_path VARCHAR(255))"
	@echo "  RETURNS BLOB SUB_TYPE TEXT"
	@echo "  EXTERNAL NAME 'json_udfn!json_value'"
	@echo "  ENGINE UDR;"

test: all
	@echo "Testing JSON_VALUE UDR..."
	@echo "Note: This test requires a Firebird 4 SuperServer instance."
	@echo "Use the test SQL in test_json.sql with isql-fb."

clean:
	rm -f $(SO_FILE) *.o

help:
	@echo "Firebird JSON Value UDR Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build the shared object (json_udfn.so)"
	@echo "  install - Install to Firebird UDR plugin directory ($(UDR_DIR))"
	@echo "  test    - Show test instructions"
	@echo "  clean   - Remove build artifacts"
	@echo ""
	@echo "Environment Variables:"
	@echo "  UDR_DIR         - Override UDR plugin directory (default: /plugins/udr)"
	@echo "  FIREBIRD_CONFIG - Path to firebird-config utility (auto-detected)"
