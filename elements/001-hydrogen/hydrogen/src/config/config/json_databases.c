/*
 * Databases configuration JSON parsing
 */

// Core system headers
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Project headers
#include "json_databases.h"
#include "../databases/config_databases.h"
#include "../config.h"
#include "../env/config_env.h"
#include "../config_utils.h"
#include "../types/config_bool.h"
#include "../types/config_int.h"
#include "../../logging/logging.h"

// Helper to process and log an environment variable
static char* process_env_var(const char* key, json_t* value, const char* default_value, bool is_sensitive, bool is_default __attribute__((unused))) {
    char* result = get_config_string_with_env(key, value, default_value);
    if (!result) return NULL;

    // Check if this is an environment variable
    if (strncmp(result, "${env.", 6) == 0) {
        const char* env_name = result + 6;
        size_t len = strlen(env_name);
        if (len > 1 && env_name[len-1] == '}') {
            // Safely extract env var name
            char env_var[256];
            size_t copy_len = len - 1;
            if (copy_len >= sizeof(env_var)) {
                copy_len = sizeof(env_var) - 1;
            }
            memcpy(env_var, env_name, copy_len);
            env_var[copy_len] = '\0';
            
            const char* env_val = getenv(env_var);
            if (env_val) {
                if (is_sensitive) {
                    log_this("Config", "    ― %s: $%s: %.5s... (%s)", LOG_LEVEL_STATE,
                            key, env_var, env_val, is_default ? "default" : "from config");
                } else {
                    log_this("Config", "    ― %s: $%s: %s (%s)", LOG_LEVEL_STATE,
                            key, env_var, env_val, is_default ? "default" : "from config");
                }
            } else {
                log_this("Config", "    ― %s: $%s: not set (%s)", LOG_LEVEL_STATE,
                        key, env_var, is_default ? "default" : "from config");
            }
        }
    } else if (result[0] != '\0') {
        // Non-empty non-env value
        if (is_sensitive) {
            log_this("Config", "    ― %s: %.5s... (%s)", LOG_LEVEL_STATE,
                    key, result, is_default ? "default" : "from config");
        } else {
            log_this("Config", "    ― %s: %s (%s)", LOG_LEVEL_STATE,
                    key, result, is_default ? "default" : "from config");
        }
    } else {
        // Empty value
        log_this("Config", "    ― %s: not set (%s)", LOG_LEVEL_STATE,
                key, is_default ? "default" : "from config");
    }

    return result;
}

// Helper to process a database connection from JSON
static void process_database_connection(json_t* conn_obj, DatabaseConnection* conn, bool is_default __attribute__((unused))) {
    if (!conn_obj || !json_is_object(conn_obj)) {
        // Log default values
        log_this("Config", "  ― %s (default)", LOG_LEVEL_STATE, conn->name);
        if (conn->enabled) {
            // Type
            log_this("Config", "    ― Type: %s (default)", LOG_LEVEL_STATE, conn->type);

            // Process default env vars
            process_env_var("Database", NULL, "${env.ACURANZO_DATABASE}", false, true);
            process_env_var("Host", NULL, "${env.ACURANZO_DB_HOST}", false, true);
            process_env_var("Port", NULL, "${env.ACURANZO_DB_PORT}", false, true);
            process_env_var("User", NULL, "${env.ACURANZO_DB_USER}", true, true);
            process_env_var("Pass", NULL, "${env.ACURANZO_DB_PASS}", true, true);

            // Workers
            log_this("Config", "    ― Workers: %d (default)", LOG_LEVEL_STATE, conn->workers);
        }
        log_this("Config", "    ― Enabled: %s (default)", LOG_LEVEL_STATE,
                conn->enabled ? "true" : "false");
        return;
    }

    // Log from config values
    log_this("Config", "  ― %s (from config)", LOG_LEVEL_STATE, conn->name);

    // Type
    json_t* type = json_object_get(conn_obj, "Type");
    if (json_is_string(type)) {
        free(conn->type);
        conn->type = strdup(json_string_value(type));
    }
    log_this("Config", "    ― Type: %s %s", LOG_LEVEL_STATE,
            conn->type, json_is_string(type) ? "(from config)" : "(default)");

    if (conn->enabled) {
        // Process each parameter
        const char* params[] = {"Database", "Host", "Port", "User", "Pass"};
        const char* env_vars[] = {
            "${env.ACURANZO_DATABASE}",
            "${env.ACURANZO_DB_HOST}",
            "${env.ACURANZO_DB_PORT}",
            "${env.ACURANZO_DB_USER}",
            "${env.ACURANZO_DB_PASS}"
        };
        bool sensitive[] = {false, false, false, true, true};

        for (int i = 0; i < 5; i++) {
            json_t* param = json_object_get(conn_obj, params[i]);
            char** param_ptr = NULL;
            
            switch (i) {
                case 0: param_ptr = &conn->database; break;
                case 1: param_ptr = &conn->host; break;
                case 2: param_ptr = &conn->port; break;
                case 3: param_ptr = &conn->user; break;
                case 4: param_ptr = &conn->pass; break;
            }

            char* new_val = process_env_var(params[i], param, env_vars[i], sensitive[i], !json_is_string(param));
            if (new_val) {
                free(*param_ptr);
                *param_ptr = new_val;
            }
        }

        // Workers
        json_t* workers = json_object_get(conn_obj, "Workers");
        if (json_is_integer(workers)) {
            conn->workers = json_integer_value(workers);
        }
        log_this("Config", "    ― Workers: %d %s", LOG_LEVEL_STATE,
                conn->workers, json_is_integer(workers) ? "(from config)" : "(default)");
    }

    // Enabled
    json_t* enabled = json_object_get(conn_obj, "Enabled");
    if (json_is_boolean(enabled)) {
        conn->enabled = json_boolean_value(enabled);
    }
    log_this("Config", "    ― Enabled: %s %s", LOG_LEVEL_STATE,
            conn->enabled ? "true" : "false",
            json_is_boolean(enabled) ? "(from config)" : "(default)");
}

// Helper to log a database connection configuration
static void log_database_connection(const DatabaseConnection* conn, bool is_default) {
    char indent[16];
    snprintf(indent, sizeof(indent), "  ― %s", conn->name);
    log_this("Config", "%s%s", LOG_LEVEL_STATE, indent, is_default ? " (default)" : "");

    if (conn->enabled) {
        // Environment variables are already logged during processing
        // Only log non-sensitive defaults here
        if (is_default) {
            log_this("Config", "    ― Type: %s (default)", LOG_LEVEL_STATE, conn->type);
            log_this("Config", "    ― Database: $%s_DATABASE: not set", LOG_LEVEL_STATE, conn->name);
            log_this("Config", "    ― Host: $%s_DB_HOST: not set", LOG_LEVEL_STATE, conn->name);
            log_this("Config", "    ― Port: $%s_DB_PORT: not set", LOG_LEVEL_STATE, conn->name);
            log_this("Config", "    ― User: $%s_DB_USER: not set", LOG_LEVEL_STATE, conn->name);
            log_this("Config", "    ― Workers: %d (default)", LOG_LEVEL_STATE, conn->workers);
        }
    }
    if (is_default) {
        log_this("Config", "    ― Enabled: %s (default)", LOG_LEVEL_STATE,
                conn->enabled ? "true" : "false");
    }
}

bool load_json_databases(json_t* root, AppConfig* config) {
    // Initialize with defaults
    init_database_config(&config->databases);

    // Process Databases section if present
    json_t* databases = json_object_get(root, "Databases");
    bool using_defaults = !json_is_object(databases);

    log_this("Config", "Databases%s", LOG_LEVEL_STATE, using_defaults ? " *" : "");

    if (!using_defaults) {
        // Process DefaultWorkers
        json_t* default_workers = json_object_get(databases, "DefaultWorkers");
        if (json_is_integer(default_workers)) {
            config->databases.default_workers = json_integer_value(default_workers);
        }
        log_this("Config", "― DefaultWorkers: %d%s", LOG_LEVEL_STATE,
                 config->databases.default_workers,
                 !json_is_integer(default_workers) ? " *" : "");

        // Process Connections section
        json_t* connections = json_object_get(databases, "Connections");
        if (json_is_object(connections)) {
            log_this("Config", "― Connections", LOG_LEVEL_STATE);

            // Process each known database
            for (int i = 0; i < MAX_DATABASES; i++) {
                json_t* conn_obj = json_object_get(connections, KNOWN_DATABASES[i]);
                if (json_is_object(conn_obj)) {
                    process_database_connection(conn_obj, &config->databases.connections[i], false);
                    log_database_connection(&config->databases.connections[i], false);
                } else {
                    log_database_connection(&config->databases.connections[i], true);
                }
            }
        } else {
            log_this("Config", "― Connections *", LOG_LEVEL_STATE);
            // Log default connections
            for (int i = 0; i < MAX_DATABASES; i++) {
                log_database_connection(&config->databases.connections[i], true);
            }
        }
    } else {
        log_this("Config", "― Status: Section missing, using defaults", LOG_LEVEL_ALERT);
        log_this("Config", "― DefaultWorkers: %d *", LOG_LEVEL_STATE, config->databases.default_workers);
        log_this("Config", "― Connections *", LOG_LEVEL_STATE);
        // Log default connections
        for (int i = 0; i < MAX_DATABASES; i++) {
            log_database_connection(&config->databases.connections[i], true);
        }
    }

    return true;
}