/*
 * Database Connection String Parsing Implementation
 *
 * Implements parsing and management of database connection strings
 * for different database engines (PostgreSQL, MySQL, SQLite, DB2).
 */

#include "../hydrogen.h"
#include <assert.h>

// Local includes
#include "database.h"
#include "database_connstring.h"

/*
 * Parse connection string into ConnectionConfig
 * Supports formats like: postgresql://user:pass@host:port/database
 */
ConnectionConfig* parse_connection_string(const char* conn_string) {
    if (!conn_string) return NULL;

    ConnectionConfig* config = calloc(1, sizeof(ConnectionConfig));
    if (!config) return NULL;

    // Copy the connection string
    config->connection_string = strdup(conn_string);
    if (!config->connection_string) {
        free(config);
        return NULL;
    }

    // Parse PostgreSQL connection string format: postgresql://user:pass@host:port/database
    if (strncmp(conn_string, "postgresql://", 13) == 0) {
        const char* after_proto = conn_string + 13; // Skip "postgresql://"

        // Find @ symbol to separate user:pass from host:port/database
        const char* at_pos = strchr(after_proto, '@');
        if (at_pos) {
            // Parse user:pass part
            char user_pass[256] = {0};
            size_t user_pass_len = (size_t)(at_pos - after_proto);
            if (user_pass_len < sizeof(user_pass) && user_pass_len > 0) {
                strncpy(user_pass, after_proto, user_pass_len);
                user_pass[user_pass_len] = '\0';

                // Split user:pass
                char* colon_pos = strchr(user_pass, ':');
                if (colon_pos) {
                    *colon_pos = '\0';
                    config->username = strdup(user_pass);
                    if (!config->username) goto cleanup;
                    config->password = strdup(colon_pos + 1);
                    if (!config->password) goto cleanup;
                } else {
                    config->username = strdup(user_pass);
                    if (!config->username) goto cleanup;
                }
            }

            // Parse host:port/database part
            const char* host_start = at_pos + 1;
            const char* slash_pos = strchr(host_start, '/');
            if (slash_pos) {
                // Parse host:port
                char host_port[256] = {0};
                size_t host_port_len = (size_t)(slash_pos - host_start);
                if (host_port_len < sizeof(host_port) && host_port_len > 0) {
                    strncpy(host_port, host_start, host_port_len);
                    host_port[host_port_len] = '\0';

                    // Split host:port
                    char* colon_pos = strchr(host_port, ':');
                    if (colon_pos) {
                        *colon_pos = '\0';
                        config->host = strdup(host_port);
                        if (!config->host) goto cleanup;
                        config->port = atoi(colon_pos + 1);
                    } else {
                        config->host = strdup(host_port);
                        if (!config->host) goto cleanup;
                        config->port = 5432; // Default PostgreSQL port
                    }
                }

                // Parse database
                config->database = strdup(slash_pos + 1);
                if (!config->database) goto cleanup;
            }
        }
    }
    // Parse MySQL connection string format: mysql://user:pass@host:port/database
    else if (strncmp(conn_string, "mysql://", 8) == 0) {
        const char* after_proto = conn_string + 8; // Skip "mysql://"

        // Find @ symbol to separate user:pass from host:port/database
        const char* at_pos = strchr(after_proto, '@');
        if (at_pos) {
            // Parse user:pass part
            char user_pass[256] = {0};
            size_t user_pass_len = (size_t)(at_pos - after_proto);
            if (user_pass_len < sizeof(user_pass) && user_pass_len > 0) {
                strncpy(user_pass, after_proto, user_pass_len);
                user_pass[user_pass_len] = '\0';

                // Split user:pass
                char* colon_pos = strchr(user_pass, ':');
                if (colon_pos) {
                    *colon_pos = '\0';
                    config->username = strdup(user_pass);
                    if (!config->username) goto cleanup;
                    config->password = strdup(colon_pos + 1);
                    if (!config->password) goto cleanup;
                } else {
                    config->username = strdup(user_pass);
                    if (!config->username) goto cleanup;
                }
            }

            // Parse host:port/database part
            const char* host_start = at_pos + 1;
            const char* slash_pos = strchr(host_start, '/');
            if (slash_pos) {
                // Parse host:port
                char host_port[256] = {0};
                size_t host_port_len = (size_t)(slash_pos - host_start);
                if (host_port_len < sizeof(host_port) && host_port_len > 0) {
                    strncpy(host_port, host_start, host_port_len);
                    host_port[host_port_len] = '\0';

                    // Split host:port
                    char* colon_pos = strchr(host_port, ':');
                    if (colon_pos) {
                        *colon_pos = '\0';
                        config->host = strdup(host_port);
                        if (!config->host) goto cleanup;
                        config->port = atoi(colon_pos + 1);
                    } else {
                        config->host = strdup(host_port);
                        if (!config->host) goto cleanup;
                        config->port = 3306; // Default MySQL port
                    }
                }

                // Parse database
                config->database = strdup(slash_pos + 1);
                if (!config->database) goto cleanup;
            }
        }
    }
    // Parse DB2 ODBC connection string format: DRIVER={...};DATABASE=...;HOSTNAME=...;PORT=...;PROTOCOL=...;UID=...;PWD=...
    else if (strstr(conn_string, "DRIVER=") != NULL && strstr(conn_string, "DATABASE=") != NULL) {
        // Parse key=value pairs separated by semicolons
        char* conn_str_copy = strdup(conn_string);
        if (!conn_str_copy) goto cleanup;

        char* token = strtok(conn_str_copy, ";");
        while (token != NULL) {
            // Skip whitespace
            while (*token == ' ') token++;

            // Find the = separator
            char* equals_pos = strchr(token, '=');
            if (equals_pos) {
                *equals_pos = '\0';
                const char* key = token;
                const char* value = equals_pos + 1;

                // Remove quotes around values if present
                if (*value == '{' && value[strlen(value) - 1] == '}') {
                    // Handle {value} format for DRIVER
                    value++;
                    char* end_quote = strrchr((char*)value, '}');
                    if (end_quote) *end_quote = '\0';
                } else if (*value == '"' && value[strlen(value) - 1] == '"') {
                    // Handle "value" format
                    value++;
                    char* end_quote = strrchr((char*)value, '"');
                    if (end_quote) *end_quote = '\0';
                }

                if (strcmp(key, "DATABASE") == 0) {
                    config->database = strdup(value);
                    if (!config->database) {
                        free(conn_str_copy);
                        goto cleanup;
                    }
                } else if (strcmp(key, "HOSTNAME") == 0) {
                    config->host = strdup(value);
                    if (!config->host) {
                        free(conn_str_copy);
                        goto cleanup;
                    }
                } else if (strcmp(key, "PORT") == 0) {
                    config->port = atoi(value);
                } else if (strcmp(key, "UID") == 0) {
                    config->username = strdup(value);
                    if (!config->username) {
                        free(conn_str_copy);
                        goto cleanup;
                    }
                } else if (strcmp(key, "PWD") == 0) {
                    config->password = strdup(value);
                    if (!config->password) {
                        free(conn_str_copy);
                        goto cleanup;
                    }
                }
            }
            token = strtok(NULL, ";");
        }
        free(conn_str_copy);
    }

    // Set defaults if not specified - with proper error handling
    if (!config->host) {
        config->host = strdup("localhost");
        if (!config->host) goto cleanup;
    }
    if (!config->port) config->port = 5432;

    // Handle database field based on connection string format
    if (!config->database) {
        // Check if this is a SQLite file path (doesn't match PostgreSQL or DB2 formats)
        if (conn_string && strncmp(conn_string, "postgresql://", 13) != 0 &&
            !strstr(conn_string, "DATABASE=")) {
            // This is likely a SQLite file path - use it as the database
            config->database = strdup(conn_string);
            if (!config->database) goto cleanup;
        } else {
            // Default to postgres for PostgreSQL connections
            config->database = strdup("postgres");
            if (!config->database) goto cleanup;
        }
    }
    if (!config->username) {
        config->username = strdup("");
        if (!config->username) goto cleanup;
    }
    if (!config->password) {
        config->password = strdup("");
        if (!config->password) goto cleanup;
    }

    config->timeout_seconds = 30; // Default timeout
    config->ssl_enabled = false;  // Default SSL setting

    return config;

cleanup:
    // Clean up any allocated memory on failure
    if (config->host) free(config->host);
    if (config->database) free(config->database);
    if (config->username) free(config->username);
    if (config->password) free(config->password);
    if (config->connection_string) free(config->connection_string);
    free(config);
    return NULL;
}

/*
 * Free ConnectionConfig and all its allocated strings
 */
void free_connection_config(ConnectionConfig* config) {
    if (!config) return;

    // Only free allocated string fields with additional safety checks
    if (config->host && (uintptr_t)config->host > 0x1000) free(config->host);
    if (config->database && (uintptr_t)config->database > 0x1000) free(config->database);
    if (config->username && (uintptr_t)config->username > 0x1000) free(config->username);
    if (config->password && (uintptr_t)config->password > 0x1000) free(config->password);
    if (config->connection_string && (uintptr_t)config->connection_string > 0x1000) free(config->connection_string);

    // SSL fields are not allocated in parse_connection_string, so don't free them
    // if (config->ssl_cert_path) free(config->ssl_cert_path);
    // if (config->ssl_key_path) free(config->ssl_key_path);
    // if (config->ssl_ca_path) free(config->ssl_ca_path);

    free(config);
}