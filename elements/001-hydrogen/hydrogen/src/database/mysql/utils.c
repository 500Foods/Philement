/*
 * MySQL Database Engine - Utility Functions Implementation
 *
 * Implements MySQL utility functions.
 */
#include <src/hydrogen.h>
#include <src/database/database.h>
#include "types.h"
#include "connection.h"
#include "utils.h"

/*
 * Utility Functions
 */

char* mysql_get_connection_string(const ConnectionConfig* config) {
    if (!config) return NULL;

    char* conn_str = calloc(1, 1024);
    if (!conn_str) return NULL;

    if (config->connection_string) {
        strcpy(conn_str, config->connection_string);
    } else {
        snprintf(conn_str, 1024,
                  "mysql://%s:%s@%s:%d/%s",
                  config->username ? config->username : "",
                  config->password ? config->password : "",
                  config->host ? config->host : "localhost",
                  config->port > 0 ? config->port : 3306,
                  config->database ? config->database : "");
    }

    return conn_str;
}

bool mysql_validate_connection_string(const char* connection_string) {
    if (!connection_string) return false;

    // Basic validation - check for mysql:// prefix
    return strncmp(connection_string, "mysql://", 8) == 0;
}

char* mysql_escape_string(const DatabaseHandle* connection, const char* input) {
    if (!connection || !input || connection->engine_type != DB_ENGINE_MYSQL) {
        return NULL;
    }

    // MySQL string escaping - simple implementation
    size_t escaped_len = strlen(input) * 2 + 1;
    char* escaped = calloc(1, escaped_len);
    if (!escaped) return NULL;

    // Simple escaping - replace single quotes and backslashes
    const char* src = input;
    char* dst = escaped;
    while (*src) {
        if (*src == '\'' || *src == '\\') {
            *dst++ = '\\';
        }
        *dst++ = *src;
        src++;
    }

    return escaped;
}

int mysql_json_escape_string(const char* input, char* output, size_t output_size) {
    if (!input || !output || output_size < 2) {
        return -1;
    }

    const char* src = input;
    char* dst = output;
    size_t available = output_size - 1; // Reserve space for null terminator

    while (*src && available > 0) {
        if (*src == '"' || *src == '\\') {
            if (available < 2) break;
            *dst++ = '\\';
            *dst++ = (*src == '"') ? '"' : '\\';
            available -= 2;
        } else if (*src == '\n') {
            if (available < 2) break;
            *dst++ = '\\';
            *dst++ = 'n';
            available -= 2;
        } else if (*src == '\r') {
            if (available < 2) break;
            *dst++ = '\\';
            *dst++ = 'r';
            available -= 2;
        } else if (*src == '\t') {
            if (available < 2) break;
            *dst++ = '\\';
            *dst++ = 't';
            available -= 2;
        } else {
            *dst++ = *src;
            available--;
        }
        src++;
    }

    *dst = '\0';

    // Return -1 if we couldn't fit everything
    if (*src != '\0') {
        return -1;
    }

    // Cast pointer difference to int safely
    ptrdiff_t diff = dst - output;
    return (int)diff;
}