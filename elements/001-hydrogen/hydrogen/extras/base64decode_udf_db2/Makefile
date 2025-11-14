# Makefile for TEST.BASE64DECODE (chunked C UDF + SQL wrapper)
# Usage:
#   export DB2DIR=/opt/ibm/db2/V11.5        # adjust for your install
#   make                                    # builds .so
#   make install                            # copies .so to sqllib/function
#   make register DB=HYDROTST SCHEMA=TEST   # creates functions + smoke test
#   make test DB=HYDROTST                   # run tests again
#   make clean

DB2DIR ?= /opt/ibm/db2/V11.5
SCHEMA  ?= TEST
DB      ?= HYDROTST

CC      := gcc
CFLAGS  := -fPIC \
           -shared  \
		   -O2 \
		   -I"/opt/ibm/db2/V11.1/include" \
		   -fPIC 
SO_DIR  := /home/db2inst1/sqllib/function
SO_FILE := base64_chunk_udf.so
SRC     := base64_chunk_udf.c

.PHONY: all install register test clean

all: $(SO_FILE)

$(SO_FILE): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

install: $(SO_FILE)
	mkdir -p "$(SO_DIR)"
	cp -f "$(SO_FILE)" "$(SO_DIR)/"
	chmod 755 "$(SO_DIR)/$(SO_FILE)"

register:
	@echo "Connecting to $(DB) and registering UDFs in schema $(SCHEMA)..."
	@tmpfile=$$(mktemp /tmp/install_base64decode.XXXXXX.clp); \
	  { \
	    echo "CONNECT TO $(DB)@"; \
	    echo "SET CURRENT SCHEMA $(SCHEMA)@"; \
	    sed -e "s/\$${SCHEMA}/$(SCHEMA)/g" install_base64decode.clp; \
	  } > $$tmpfile; \
	  db2 -td@ -vf $$tmpfile; \
	  rm -f $$tmpfile
	@echo "Done."

test:
	db2 -v "CONNECT TO $(DB)"
	db2 -tv "VALUES $(SCHEMA).BASE64_DECODE_CHUNK('SGVsbG8gV29ybGQh')"
	db2 -tv "VALUES CAST($(SCHEMA).BASE64DECODE('SGVsbG8gV29ybGQh') AS VARCHAR(100))"

clean:
	rm -f $(SO_FILE)

