# Makefile for DB2 Brotli Decompression UDF
# Usage:
#   export DB2DIR=/opt/ibm/db2/V12.1       # adjust for your install
#   make                                    # builds .so
#   make install                            # copies .so to sqllib/function
#   make register DB=HYDROTST SCHEMA=TEST   # creates functions + smoke test
#   make test DB=HYDROTST                   # run tests again
#   make clean

DB2DIR ?= /opt/ibm/db2/V12.1
SCHEMA  ?= TEST
DB      ?= HYDROTST

CC      := gcc
CFLAGS  := -fpic -O2 -I"$(DB2DIR)/include" -D_REENTRANT
LDFLAGS := -shared -Wl,-rpath,$(DB2DIR)/lib64
LIBS    := -L$(DB2DIR)/lib64 -ldb2 -lpthread -lbrotlidec
SO_DIR  := /home/db2inst1/sqllib/function
SO_FILE := brotli_decompress.so
SRC     := brotli_decompress.c

.PHONY: all install register test clean

all: $(SO_FILE)

$(SO_FILE): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC)
	$(CC) $(LDFLAGS) -o $@ brotli_decompress.o $(LIBS)
	rm -f brotli_decompress.o

install: $(SO_FILE)
	mkdir -p "$(SO_DIR)"
	cp -f "$(SO_FILE)" "$(SO_DIR)/"
	chmod 755 "$(SO_DIR)/$(SO_FILE)"

register:
	@echo "Connecting to $(DB) and registering Brotli decompression UDFs in schema $(SCHEMA)..."
	@tmpfile=$$(mktemp /tmp/install_brotli_decompress.XXXXXX.clp); \
	  { \
	    echo "CONNECT TO $(DB)@"; \
	    echo "SET CURRENT SCHEMA $(SCHEMA)@"; \
	    sed -e "s/\$${SCHEMA}/$(SCHEMA)/g" install_brotli_decompress.clp; \
	  } > $$tmpfile; \
	  db2 -td@ -vf $$tmpfile; \
	  rm -f $$tmpfile
	@echo "Done."

test:
	db2 -v "CONNECT TO $(DB)"
	db2 -tv "VALUES $(SCHEMA).HELIUM_BROTLI_CHECK()"
	@echo "Testing Brotli decompression with sample data..."
	@echo "Note: Actual decompression tests require compressed data"

clean:
	rm -f $(SO_FILE) *.o