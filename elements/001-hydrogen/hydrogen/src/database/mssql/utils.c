/*
  * MSSQL Database Engine - Utility Functions Implementation
  *
  * Implements MSSQL utility functions.
  */
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/database_serialize.h>
#include "types.h"
#include "utils.h"

// Utility Functions
char* mssql_get_connection_string(const ConnectionConfig* config) {
    if (!config) return NULL;

    char* conn_str = calloc(1, 1024);
    if (!conn_str) return NULL;

    if (config->connection_string) {
        strcpy(conn_str, config->connection_string);
    } else {
        int port = config->port > 0 ? config->port : 1433;
        const char* driver = getenv("MSSQL_ODBC_DRIVER");
        char driver_bracketed[256];
        if (!driver || *driver == '\0') {
            driver = "ODBC Driver 18 for SQL Server";
        }
        if (driver[0] == '{') {
            snprintf(driver_bracketed, sizeof(driver_bracketed), "%s", driver);
        } else {
            snprintf(driver_bracketed, sizeof(driver_bracketed), "{%s}", driver);
        }
        snprintf(conn_str, 1024,
                 "DRIVER=%s;SERVER=%s,%d;DATABASE=%s;UID=%s;PWD=%s;",
                 driver_bracketed,
                 config->host ? config->host : "localhost",
                 port,
                 config->database ? config->database : "",
                 config->username ? config->username : "",
                 config->password ? config->password : "");
        if (strstr(driver_bracketed, "FreeTDS") != NULL) {
            size_t used = strlen(conn_str);
            snprintf(conn_str + used, 1024 - used, "TDS_VERSION=7.4;");
        }
    }

    // Pin unqualified DDL/queries to the configured schema so migrations
    // and ad-hoc SELECTs from a shared user account don't bleed across schemas.
    // Config::Schema wins over any SCHEMA= already present in a provided
    // connection string, so we strip a pre-existing one before appending ours.
    if (config->schema && *config->schema) {
        char* existing = strstr(conn_str, "SCHEMA=");
        if (existing) {
            char* end = strchr(existing, ';');
            if (end) {
                memmove(existing, end + 1, strlen(end + 1) + 1);
            } else {
                *existing = '\0';
            }
        }
        size_t used = strlen(conn_str);
        if (used > 0 && conn_str[used - 1] != ';') {
            if (used + 1 < 1024) { conn_str[used++] = ';'; conn_str[used] = '\0'; }
        }
        snprintf(conn_str + used, (size_t)(1024 - used),
                 "SCHEMA=%s;", config->schema);
    }

    return conn_str;
}

bool mssql_validate_connection_string(const char* connection_string) {
    if (!connection_string) return false;

    // Basic validation - check for required MSSQL connection string components
    size_t len = strlen(connection_string);
    if (len == 0 || len > 4096) return false;

    // Check for basic MSSQL ODBC connection string format
    // Should contain DRIVER= and SERVER= at minimum
    return strstr(connection_string, "DRIVER=") != NULL &&
           strstr(connection_string, "SERVER=") != NULL;
}

void mssql_parse_connection_string(const char* conn_str, char* server, char* username, char* password, char* database) {
    if (!conn_str || !server || !username || !password || !database) return;

    // Initialize outputs
    server[0] = '\0';
    username[0] = '\0';
    password[0] = '\0';
    database[0] = '\0';

    // Parse key=value pairs separated by semicolons
    char* conn_copy = strdup(conn_str);
    if (!conn_copy) return;

    char* token = strtok(conn_copy, ";");
    while (token) {
        char* equals = strchr(token, '=');
        if (equals) {
            *equals = '\0';
            char* key = token;
            const char* value = equals + 1;

            if (strcasecmp(key, "SERVER") == 0 || strcasecmp(key, "ADDR") == 0) {
                // SERVER may be host,port format
                strncpy(server, value, 255);
            } else if (strcasecmp(key, "UID") == 0 || strcasecmp(key, "USER") == 0) {
                strncpy(username, value, 127);
            } else if (strcasecmp(key, "PWD") == 0 || strcasecmp(key, "PASSWORD") == 0) {
                strncpy(password, value, 127);
            } else if (strcasecmp(key, "DATABASE") == 0 || strcasecmp(key, "DB") == 0) {
                strncpy(database, value, 127);
            }
        }
        token = strtok(NULL, ";");
    }

    free(conn_copy);
}

char* mssql_escape_string(const DatabaseHandle* connection, const char* input) {
    if (!connection || !input || connection->engine_type != DB_ENGINE_MSSQL) {
        return NULL;
    }

    // MSSQL string escaping - double up single quotes
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
