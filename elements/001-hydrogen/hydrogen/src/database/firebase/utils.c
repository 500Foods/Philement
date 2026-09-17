/*
 * Firebase engine - connection string build/validate and SQL escaping.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include "types.h"
#include "utils.h"

bool firebase_host_is_production(const char* host) {
    if (!host || !*host) {
        return false;
    }
    return strcmp(host, FIREBASE_DEFAULT_PRODUCTION_HOST) == 0;
}

bool firebase_config_is_emulator(const ConnectionConfig* config) {
    if (!config) {
        return false;
    }
    if (config->connection_string && strstr(config->connection_string, "emulator=1")) {
        return true;
    }
    if (firebase_host_is_production(config->host)) {
        return false;
    }
    if (config->host) {
        if (strcmp(config->host, "127.0.0.1") == 0 ||
            strcmp(config->host, "localhost") == 0 ||
            strcmp(config->host, "::1") == 0) {
            return true;
        }
    }
    if (config->port == FIREBASE_DEFAULT_EMULATOR_PORT) {
        return true;
    }
    if (!config->host || !*config->host) {
        return true;
    }
    return false;
}

const char* firebase_resolved_project(const ConnectionConfig* config) {
    if (!config) {
        return NULL;
    }
    if (config->username && *config->username) {
        return config->username;
    }
    return NULL;
}

const char* firebase_resolved_database(const ConnectionConfig* config) {
    if (config && config->database && *config->database) {
        return config->database;
    }
    return FIREBASE_DEFAULT_DATABASE;
}

const char* firebase_resolved_host(const ConnectionConfig* config) {
    if (config && config->host && *config->host) {
        return config->host;
    }
    if (config && firebase_host_is_production(config->host)) {
        return FIREBASE_DEFAULT_PRODUCTION_HOST;
    }
    return FIREBASE_DEFAULT_EMULATOR_HOST;
}

int firebase_resolved_port(const ConnectionConfig* config) {
    if (config && config->port > 0) {
        return config->port;
    }
    if (config && firebase_config_is_emulator(config)) {
        return FIREBASE_DEFAULT_EMULATOR_PORT;
    }
    if (config && firebase_host_is_production(config->host)) {
        return FIREBASE_DEFAULT_PRODUCTION_PORT;
    }
    return FIREBASE_DEFAULT_EMULATOR_PORT;
}

char* firebase_get_connection_string(const ConnectionConfig* config) {
    if (!config) {
        return NULL;
    }

    char* conn_str = calloc(1, 1024);
    if (!conn_str) {
        return NULL;
    }

    if (config->connection_string && *config->connection_string) {
        snprintf(conn_str, 1024, "%s", config->connection_string);
        return conn_str;
    }

    const char* project = firebase_resolved_project(config);
    if (!project) {
        free(conn_str);
        return NULL;
    }
    const char* database = firebase_resolved_database(config);
    const char* host = firebase_resolved_host(config);
    int port = firebase_resolved_port(config);
    bool emulator = firebase_config_is_emulator(config);

    int written = snprintf(conn_str, 1024,
                           "firebase://%s/%s?host=%s&port=%d%s",
                           project, database, host, port,
                           emulator ? "&emulator=1" : "");
    if (written < 0 || written >= 1024) {
        free(conn_str);
        return NULL;
    }
    return conn_str;
}

bool firebase_validate_connection_string(const char* connection_string) {
    if (!connection_string) {
        return false;
    }
    if (strncmp(connection_string, "firebase://", 11) != 0) {
        return false;
    }
    const char* rest = connection_string + 11;
    if (!*rest) {
        return false;
    }
    const char* slash = strchr(rest, '/');
    if (!slash || slash == rest) {
        return false;
    }
    if (*(slash + 1) == '\0' || *(slash + 1) == '?') {
        return false;
    }
    return true;
}

char* firebase_escape_string(const DatabaseHandle* connection, const char* input) {
    if (!connection || !input || connection->engine_type != DB_ENGINE_FIREBASE) {
        return NULL;
    }

    size_t input_len = strlen(input);
    char* escaped = calloc(1, input_len * 2 + 1);
    if (!escaped) {
        return NULL;
    }

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
