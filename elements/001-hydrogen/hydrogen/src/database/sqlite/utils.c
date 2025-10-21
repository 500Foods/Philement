/*
  * SQLite Database Engine - Utility Functions Implementation
  *
  * Implements SQLite utility functions.
  */
#include <src/hydrogen.h>
#include <src/database/database.h>
#include "types.h"
#include "utils.h"

// Utility Functions
char* sqlite_get_connection_string(const ConnectionConfig* config) {
    if (!config) return NULL;

    char* conn_str = calloc(1, 1024);
    if (!conn_str) return NULL;

    if (config->connection_string) {
        strcpy(conn_str, config->connection_string);
    } else {
        // SQLite uses file paths, not traditional connection strings
        if (config->database && strlen(config->database) > 0) {
            strcpy(conn_str, config->database);
        } else {
            strcpy(conn_str, ":memory:"); // Default to in-memory database
        }
    }

    return conn_str;
}

bool sqlite_validate_connection_string(const char* connection_string) {
    if (!connection_string) return false;

    // SQLite connection strings are typically file paths or ":memory:"
    // Basic validation - check if it's not empty and doesn't contain suspicious characters
    size_t len = strlen(connection_string);
    if (len == 0) return false;

    // Allow :memory: and file paths
    if (strcmp(connection_string, ":memory:") == 0) return true;

    // Check for basic file path validity (no null bytes, reasonable length)
    if (len > 4096) return false; // Path too long

    // Check for null bytes in path (null bytes should only be at the end)
    for (size_t i = 0; i < len; i++) {
        if (connection_string[i] == '\0') {
            // Null byte found - this is only valid at the end of the string
            if (i < len - 1) {
                return false; // Null byte in middle of string
            }
            break; // End of string reached
        }
    }

    return true;
}

char* sqlite_escape_string(const DatabaseHandle* connection, const char* input) {
    if (!connection || !input || connection->engine_type != DB_ENGINE_SQLITE) {
        return NULL;
    }

    // SQLite string escaping - double up single quotes
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