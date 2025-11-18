/*
  * DB2 Database Engine - Utility Functions Implementation
  *
  * Implements DB2 utility functions.
  */
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/database_serialize.h>
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
        // DB2 ODBC connection string format
        // Format: DRIVER={IBM DB2 ODBC DRIVER};DATABASE=database;HOSTNAME=host;PORT=port;PROTOCOL=TCPIP;UID=username;PWD=password;
        // Note: Do NOT duplicate DATABASE= in the connection string - this causes the "DRIVER={IBM DB2 ODBC DRIVER}" to be interpreted as the database name
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

void db2_parse_connection_string(const char* conn_str, char* server, char* username, char* password, char* database) {
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

            if (strcasecmp(key, "HOSTNAME") == 0 || strcasecmp(key, "HOST") == 0) {
                strncpy(server, value, 255);
            } else if (strcasecmp(key, "PORT") == 0) {
                // Append port to server if hostname is already set
                if (server[0]) {
                    char port_str[16];
                    snprintf(port_str, sizeof(port_str), ":%s", value);
                    strncat(server, port_str, 255 - strlen(server));
                }
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