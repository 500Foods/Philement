# Makefile for Firebird Brotli Decompression UDR (Firebird 4)
# Builds libbrotli_decfn.so → EXTERNAL NAME 'brotli_decfn!brotli_decompress'

FIREBIRD_CONFIG ?= $(shell command -v firebird-config 2>/dev/null || true)

ifneq ($(FIREBIRD_CONFIG),)
    FIREBIRD_INCLUDE := $(shell $(FIREBIRD_CONFIG) --cflags 2>/dev/null || echo -I/usr/include)
    FIREBIRD_LIBS := $(shell $(FIREBIRD_CONFIG) --libs 2>/dev/null || echo -lfbclient)
else
    FIREBIRD_INCLUDE := -I/usr/include
    FIREBIRD_LIBS := -lfbclient
endif

UDR_DIR ?= $(shell \
  if [ -d /usr/lib64/firebird/plugins/udr ]; then echo /usr/lib64/firebird/plugins/udr; \
  elif [ -d /usr/lib/firebird/plugins/udr ]; then echo /usr/lib/firebird/plugins/udr; \
  elif [ -d /opt/firebird/plugins/udr ]; then echo /opt/firebird/plugins/udr; \
  else echo /usr/lib64/firebird/plugins/udr; fi)

CC      := g++
CFLAGS  := -fPIC -O2 -Wall -Wextra -std=c++17 $(FIREBIRD_INCLUDE)
LDFLAGS := -shared
LIBS    := $(FIREBIRD_LIBS) -lbrotlidec
SO_FILE := libbrotli_decfn.so
SRC     := brotli_decompress.cpp

.PHONY: all install test clean help

all: $(SO_FILE)

$(SO_FILE): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(LIBS)

install: $(SO_FILE)
	@echo "Installing $(SO_FILE) → $(UDR_DIR)/"
	install -d "$(UDR_DIR)"
	install -m 755 "$(SO_FILE)" "$(UDR_DIR)/$(SO_FILE)"
	@echo "Installed. Register with:"
	@echo "  CREATE OR ALTER FUNCTION BROTLI_DECOMPRESS(compressed BLOB)"
	@echo "  RETURNS BLOB"
	@echo "  EXTERNAL NAME 'brotli_decfn!brotli_decompress'"
	@echo "  ENGINE UDR;"

test: all
	@echo "Build OK: $(SO_FILE). After sudo make install, run test_brotli.sql"

clean:
	rm -f $(SO_FILE) *.o

help:
	@echo "Targets: all | install | test | clean"
	@echo "UDR_DIR=$(UDR_DIR)"
