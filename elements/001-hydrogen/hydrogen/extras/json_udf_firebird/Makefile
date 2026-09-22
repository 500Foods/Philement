# Makefile for Firebird JSON Value UDR (Firebird 4)
# Usage:
#   make                # builds libjson_udfn.so
#   sudo make install   # installs into Firebird UDR plugin directory
#   make clean
#
# Firebird loads EXTERNAL NAME 'json_udfn!json_value' from:
#   <firebird>/plugins/udr/libjson_udfn.so
#
# Requires: firebird-devel, jansson-devel, g++

FIREBIRD_CONFIG ?= $(shell command -v firebird-config 2>/dev/null || true)

ifneq ($(FIREBIRD_CONFIG),)
    FIREBIRD_INCLUDE := $(shell $(FIREBIRD_CONFIG) --cflags 2>/dev/null || echo -I/usr/include)
    FIREBIRD_LIBS := $(shell $(FIREBIRD_CONFIG) --libs 2>/dev/null || echo -lfbclient)
else
    FIREBIRD_INCLUDE := -I/usr/include
    FIREBIRD_LIBS := -lfbclient
endif

# Detect UDR plugin dir (Fedora/RHEL package layout first)
UDR_DIR ?= $(shell \
  if [ -d /usr/lib64/firebird/plugins/udr ]; then echo /usr/lib64/firebird/plugins/udr; \
  elif [ -d /usr/lib/firebird/plugins/udr ]; then echo /usr/lib/firebird/plugins/udr; \
  elif [ -d /opt/firebird/plugins/udr ]; then echo /opt/firebird/plugins/udr; \
  else echo /usr/lib64/firebird/plugins/udr; fi)

CC      := g++
CFLAGS  := -fPIC -O2 -Wall -Wextra -std=c++17 $(FIREBIRD_INCLUDE)
LDFLAGS := -shared
LIBS    := $(FIREBIRD_LIBS) -ljansson
# Linux Firebird prefixes module name with "lib"
SO_FILE := libjson_udfn.so
SRC     := json_value.cpp

.PHONY: all install test clean help

all: $(SO_FILE)

$(SO_FILE): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(LIBS)

install: $(SO_FILE)
	@echo "Installing $(SO_FILE) → $(UDR_DIR)/"
	install -d "$(UDR_DIR)"
	install -m 755 "$(SO_FILE)" "$(UDR_DIR)/$(SO_FILE)"
	@echo "Installed. Register with:"
	@echo "  CREATE OR ALTER FUNCTION JSON_VALUE(json_doc BLOB SUB_TYPE TEXT, json_path VARCHAR(255))"
	@echo "  RETURNS BLOB SUB_TYPE TEXT"
	@echo "  EXTERNAL NAME 'json_udfn!json_value'"
	@echo "  ENGINE UDR;"

test: all
	@echo "Build OK: $(SO_FILE)"
	@echo "After: sudo make install  then run test_json.sql with isql-fb"

clean:
	rm -f $(SO_FILE) *.o

help:
	@echo "Targets: all | install | test | clean"
	@echo "UDR_DIR=$(UDR_DIR)"
