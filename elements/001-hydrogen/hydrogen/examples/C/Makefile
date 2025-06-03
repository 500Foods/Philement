# Makefile for Hydrogen OIDC Client Examples
#
# This Makefile builds the C examples for the Hydrogen OIDC service:
# - auth_code_flow: Authorization Code flow with PKCE
# - client_credentials: Client Credentials flow 
# - password_flow: Resource Owner Password flow
#
# Usage:
#   make all         - Build all examples
#   make clean       - Remove build artifacts
#   make auth_code   - Build just the Authorization Code flow example
#   make client_cred - Build just the Client Credentials flow example
#   make password    - Build just the Password flow example
#   make help        - Display this help information

SHELL := /bin/bash

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -O2
DEBUG_CFLAGS = -g -DDEBUG

# Libraries
CURL_LIBS = -lcurl
JANSSON_LIBS = -ljansson
SSL_LIBS = -lcrypto
MICROHTTPD_LIBS = -lmicrohttpd

# Targets
all: auth_code client_cred password

auth_code: auth_code_flow.c
	$(CC) $(CFLAGS) -o auth_code_flow auth_code_flow.c $(CURL_LIBS) $(SSL_LIBS) $(MICROHTTPD_LIBS) $(JANSSON_LIBS)

client_cred: client_credentials.c
	$(CC) $(CFLAGS) -o client_credentials client_credentials.c $(CURL_LIBS) $(JANSSON_LIBS)

password: password_flow.c
	$(CC) $(CFLAGS) -o password_flow password_flow.c $(CURL_LIBS) $(JANSSON_LIBS)

# Debug builds with additional debugging symbols and flags
debug: debug_auth_code debug_client_cred debug_password

debug_auth_code: auth_code_flow.c
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -o auth_code_flow_debug auth_code_flow.c $(CURL_LIBS) $(SSL_LIBS) $(MICROHTTPD_LIBS) $(JANSSON_LIBS)

debug_client_cred: client_credentials.c
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -o client_credentials_debug client_credentials.c $(CURL_LIBS) $(JANSSON_LIBS)

debug_password: password_flow.c
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -o password_flow_debug password_flow.c $(CURL_LIBS) $(JANSSON_LIBS)

# Clean up
clean:
	rm -f auth_code_flow client_credentials password_flow
	rm -f auth_code_flow_debug client_credentials_debug password_flow_debug
	rm -f *.o
cleanish:	
	rm -f auth_code_flow_debug client_credentials_debug password_flow_debug
	rm -f *.o

# Help information
help:
	@echo "Hydrogen OIDC Client Examples Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all         - Build all examples (default)"
	@echo "  auth_code   - Build the Authorization Code flow example"
	@echo "  client_cred - Build the Client Credentials flow example"
	@echo "  password    - Build the Password flow example"
	@echo "  debug       - Build all examples with debug symbols"
	@echo "  clean       - Remove build artifacts"
	@echo "  help        - Display this help information"
	@echo ""
	@echo "Examples:"
	@echo "  make auth_code           - Build just the auth_code_flow example"
	@echo "  make debug_client_cred   - Build the client_credentials example with debug flags"
	@echo "  make clean all           - Clean and rebuild all examples"

# Special target to check if required libraries are available
check_deps:
	@echo "Checking for required libraries..."
	@which curl-config > /dev/null || (echo "Error: curl-config not found. Please install libcurl development package" && exit 1)
	@pkg-config --exists jansson || (echo "Error: jansson not found. Please install jansson development package" && exit 1)
	@pkg-config --exists libmicrohttpd || (echo "Warning: libmicrohttpd not found. Authorization Code flow example will not build" && exit 0)
	@pkg-config --exists openssl || (echo "Warning: openssl not found. Authorization Code flow example will not build" && exit 0)
	@echo "All dependencies available!"

# Make sure these targets work even if files with the same name exist
.PHONY: all auth_code client_cred password debug debug_auth_code debug_client_cred debug_password clean help check_deps