# Makefile for BASE64ENCODE (chunked C UDF + SQL wrapper)
# Usage:
#   export DB2DIR=/opt/ibm/db2/V11.5        # adjust for your install
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
LIBS    := -L$(DB2DIR)/lib64 -ldb2 -lpthread
SO_DIR  := /home/db2inst1/sqllib/function
SO_FILE := base64_encode_udf.so
SRC     := base64_encode_udf.c

.PHONY: all install register test clean

all: $(SO_FILE)

$(SO_FILE): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC)
	$(CC) $(LDFLAGS) -o $@ base64_encode_udf.o $(LIBS)
	rm -f base64_encode_udf.o

install: $(SO_FILE)
	sudo mkdir -p "$(SO_DIR)"
	sudo cp -f "$(SO_FILE)" "$(SO_DIR)/"
	sudo chmod 755 "$(SO_DIR)/$(SO_FILE)"

register:
	@echo "Connecting to $(DB) and registering UDFs in schema $(SCHEMA)..."
	@tmpfile=$$(mktemp /tmp/install_base64encode.XXXXXX.clp); \
	  { \
	    echo "CONNECT TO $(DB)@"; \
	    echo "SET CURRENT SCHEMA $(SCHEMA)@"; \
	    sed -e "s/\$${SCHEMA}/$(SCHEMA)/g" install_base64encode.clp; \
	  } > $$tmpfile; \
	  db2 -td@ -vf $$tmpfile; \
	  rm -f $$tmpfile
	@echo "Done."

test:
	db2 -v "CONNECT TO $(DB)"
	db2 -tv "VALUES CAST($(SCHEMA).BASE64ENCODE('Hello World!') AS VARCHAR(100))"
	db2 -tv "VALUES length($(SCHEMA).BASE64ENCODE('Hello World!'))"
	db2 -tv "VALUES CAST($(SCHEMA).BASE64DECODE($(SCHEMA).BASE64ENCODE('Hello World!')) AS VARCHAR(100))"

clean:
	rm -f $(SO_FILE)
