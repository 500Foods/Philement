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
	@echo "Testing short string decompression..."
	db2 -tv "VALUES CAST($(SCHEMA).BROTLI_DECOMPRESS($(SCHEMA).BASE64DECODE('jwWASGVsbG8gV29ybGQhAw==')) AS VARCHAR(100))"
	@echo "Testing medium string decompression..."
	db2 -tv "VALUES CAST(LEFT($(SCHEMA).BROTLI_DECOMPRESS($(SCHEMA).BASE64DECODE('H5kDAJwHto06/UBoOKC9hWBQx4wo6LThWqckQtn2Yq2aHkxjyK1mqoQyTpC9+j+X0xvb7uj0wA5wKK3G1DGX//+bgL0oLIIoijJtb2izcv7I1rZUzxGvMQ8rdgVmx7BpQOJtzBRorrsbWW119EN2tiodg2uoDtS/zPE+/EjO1FsWEzDPOMMNoWP7qtF7jnaV314VUnoNUw4zHuXgrFvsKRFeflByH8rsox08e6oZbErSH99qUCBT/fkQ5fdo9XRvnVHmyOLnGPNoeiKZan2+06WjrcLfGVNLs7DuFOdFG345ohqAA/Ob2/6l4vcL8QSo2zDP42yZ0TeHvcYHxNoWRtavWKzINZ4NfF0tVwPm8ykB/JHTYBwjBWVBgOTAxFfrovXNzq7Xsd/hnupf6qE21GsqC/4FRlVMWjZDbccA7Qy2/7hH2VhB2+JKhloHK+GbksQNkIeuuY2iMEo=')), 4) AS VARCHAR(4)) FROM SYSIBM.SYSDUMMY1"
	@echo "Testing large string decompression..."
	db2 -tv "VALUES CAST(LEFT($(SCHEMA).BROTLI_DECOMPRESS($(SCHEMA).BASE64DECODE('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')), 4) AS VARCHAR(4)) FROM SYSIBM.SYSDUMMY1"
	@echo "Brotli decompression tests completed successfully"

clean:
	rm -f $(SO_FILE) *.o