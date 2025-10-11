/*
  * DB2 Database Engine - Utility Functions Implementation
  *
  * Implements DB2 utility functions.
  */
#include <src/hydrogen.h>
#include "../database.h"
#include "types.h"
#include "utils.h"

// Utility Functions
char* db2_get_connection_string(const ConnectionConfig* config) {
    if (!config) return NULL;

    char* conn_str = calloc(1, 1024);
    if (!conn_str) return NULL;

    if (config->connection_string) {
        strcpy(conn_str, config->connection_string);
    } else {
        // DB2 ODBC connection string format with DRIVER specification
        // Format: DRIVER={IBM DB2 ODBC DRIVER};DATABASE=database;HOSTNAME=host;PORT=port;PROTOCOL=TCPIP;UID=username;PWD=password;
        snprintf(conn_str, 1024,
                 "DRIVER={IBM DB2 ODBC DRIVER};DATABASE=%s;HOSTNAME=%s;PORT=%d;PROTOCOL=TCPIP;UID=%s;PWD=%s;",
                 config->database ? config->database : "",
                 config->host ? config->host : "localhost",
                 config->port > 0 ? config->port : 50000,  // Default DB2 port
                 config->username ? config->username : "",
                 config->password ? config->password : "");
    }

    return conn_str;
}

bool db2_validate_connection_string(const char* connection_string) {
    if (!connection_string) return false;

    // Basic validation - check for required DB2 connection string components
    size_t len = strlen(connection_string);
    if (len == 0 || len > 4096) return false;

    // Check for basic DB2 connection string format
    // Should contain DATABASE= and HOSTNAME= at minimum
    return strstr(connection_string, "DATABASE=") != NULL &&
           strstr(connection_string, "HOSTNAME=") != NULL;
}

char* db2_escape_string(const DatabaseHandle* connection, const char* input) {
    if (!connection || !input || connection->engine_type != DB_ENGINE_DB2) {
        return NULL;
    }

    // DB2 string escaping - double up single quotes
    size_t input_len = strlen(input);
    size_t escaped_len = input_len * 2 + 1; // Worst case: every char is a quote
    char* escaped = calloc(1, escaped_len);
    if (!escaped) return NULL;

    const char* src = input;
    char* dst = escaped;
    while (*src) {
        if (*src == '\'') {
            *dst++ = '\'';
            *dst++ = '\'';
        } else {
            *dst++ = *src;
        }
        src++;
    }

    return escaped;
}