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

// Helper to process a database connection from JSON
static void process_database_connection(json_t* conn_obj, DatabaseConnection* conn, bool is_default __attribute__((unused))) {
    if (!conn_obj || !json_is_object(conn_obj)) {
        // Log default values
        log_config_item(conn->name, "", true, 1);
        if (conn->enabled) {
            log_config_item("Type", conn->type, true, 2);

            // Process default env vars
            process_config_env_var("Database", NULL, "${env.ACURANZO_DATABASE}", false, true);
            process_config_env_var("Host", NULL, "${env.ACURANZO_DB_HOST}", false, true);
            process_config_env_var("Port", NULL, "${env.ACURANZO_DB_PORT}", false, true);
            process_config_env_var("User", NULL, "${env.ACURANZO_DB_USER}", true, true);
            process_config_env_var("Pass", NULL, "${env.ACURANZO_DB_PASS}", true, true);

            log_config_item("Workers", format_int_buffer(conn->workers), true, 2);
        }
        log_config_item("Enabled", conn->enabled ? "true" : "false", true, 2);
        return;
    }

    // Log from config values
    log_config_item(conn->name, "", false, 1);

    // Type
    json_t* type = json_object_get(conn_obj, "Type");
    bool using_default_type = !json_is_string(type);
    if (!using_default_type) {
        free(conn->type);
        conn->type = strdup(json_string_value(type));
    }
    log_config_item("Type", conn->type, using_default_type, 2);

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
        const bool sensitive[] = {false, false, false, true, true};

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

            char* new_val = process_config_env_var(params[i], param, env_vars[i], 
                                                 sensitive[i], !json_is_string(param));
            if (new_val) {
                free(*param_ptr);
                *param_ptr = new_val;
            }
        }

        // Workers
        json_t* workers = json_object_get(conn_obj, "Workers");
        bool using_default_workers = !json_is_integer(workers);
        if (!using_default_workers) {
            conn->workers = json_integer_value(workers);
        }
        log_config_item("Workers", format_int_buffer(conn->workers), using_default_workers, 2);
    }

    // Enabled
    json_t* enabled = json_object_get(conn_obj, "Enabled");
    bool using_default_enabled = !json_is_boolean(enabled);
    if (!using_default_enabled) {
        conn->enabled = json_boolean_value(enabled);
    }
    log_config_item("Enabled", conn->enabled ? "true" : "false", using_default_enabled, 2);
}

bool load_json_databases(json_t* root, AppConfig* config) {
    // Initialize with defaults
    init_database_config(&config->databases);

    // Process Databases section if present
    json_t* databases = json_object_get(root, "Databases");
    bool using_defaults = !json_is_object(databases);

    log_config_section("Databases", using_defaults);

    if (!using_defaults) {
        // Process DefaultWorkers
        json_t* default_workers = json_object_get(databases, "DefaultWorkers");
        bool using_default_workers = !json_is_integer(default_workers);
        if (!using_default_workers) {
            config->databases.default_workers = json_integer_value(default_workers);
        }
        log_config_item("DefaultWorkers", 
                       format_int_buffer(config->databases.default_workers),
                       using_default_workers, 0);

        // Process Connections section
        json_t* connections = json_object_get(databases, "Connections");
        if (json_is_object(connections)) {
            log_config_item("Connections", "", false, 0);

            // Process each known database
            for (int i = 0; i < MAX_DATABASES; i++) {
                json_t* conn_obj = json_object_get(connections, KNOWN_DATABASES[i]);
                process_database_connection(conn_obj, &config->databases.connections[i], 
                                         !json_is_object(conn_obj));
            }
        } else {
            log_config_item("Connections", "", true, 0);
            // Log default connections
            for (int i = 0; i < MAX_DATABASES; i++) {
                process_database_connection(NULL, &config->databases.connections[i], true);
            }
        }
    } else {
        log_config_item("Status", "Section missing, using defaults", true, 0);
        log_config_item("DefaultWorkers", 
                       format_int_buffer(config->databases.default_workers), 
                       true, 0);
        log_config_item("Connections", "", true, 0);
        // Log default connections
        for (int i = 0; i < MAX_DATABASES; i++) {
            process_database_connection(NULL, &config->databases.connections[i], true);
        }
    }

    return true;
}