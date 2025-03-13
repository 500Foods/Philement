/*
 * Databases configuration JSON parsing include file
 * 
 * IMPORTANT: This is not a standalone C file!
 * This file is meant to be included directly inside the load_config function
 * in config.c, with all necessary variables already in scope.
 */

json_t* databases = json_object_get(root, "Databases");
if (json_is_object(databases)) {
    log_config_section_header("Databases");
    
    // DefaultWorkers 
    json_t* default_workers = json_object_get(databases, "DefaultWorkers");
    int default_workers_value = 1;  // Default value
    if (json_is_integer(default_workers)) {
        default_workers_value = json_integer_value(default_workers);
        log_config_section_item("DefaultWorkers", "%d", LOG_LEVEL_STATE, 0, 0, NULL, NULL, default_workers_value);
    } else {
        log_config_section_item("DefaultWorkers", "%d", LOG_LEVEL_STATE, 1, 0, NULL, NULL, default_workers_value);
    }
    
    json_t* connections = json_object_get(databases, "Connections");
    if (json_is_object(connections)) {
        // Get connection count and display
        size_t conn_count = json_object_size(connections);
        log_config_section_item("Connections", "%zu Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, conn_count);
        
        // Get connection names and sort them
        const char** conn_names = malloc(conn_count * sizeof(char*));
        if (conn_names) {
            size_t i = 0;
            const char* name;
            json_t* conn;
            
            // Collect connection names
            json_object_foreach(connections, name, conn) {
                conn_names[i++] = name;
            }
            
            // Sort names (simple bubble sort)
            for (size_t i = 0; i < conn_count - 1; i++) {
                for (size_t j = 0; j < conn_count - i - 1; j++) {
                    if (strcmp(conn_names[j], conn_names[j + 1]) > 0) {
                        const char* temp = conn_names[j];
                        conn_names[j] = conn_names[j + 1];
                        conn_names[j + 1] = temp;
                    }
                }
            }
            
            // Display each database connection
            for (size_t i = 0; i < conn_count; i++) {
                json_t* conn = json_object_get(connections, conn_names[i]);
                log_config_section_item(conn_names[i], "", LOG_LEVEL_STATE, 0, 1, NULL, NULL);
                
                json_t* type = json_object_get(conn, "Type");
                if (json_is_string(type)) {
                    log_config_section_item("Type", "%s", LOG_LEVEL_STATE, 0, 2, NULL, NULL, json_string_value(type));
                }
                
                // Process host with environment variable support and default
                json_t* host = json_object_get(conn, "Host");
                char* host_value = get_config_string_with_env("Host", host, "localhost");
                log_config_section_item("Host", "%s", LOG_LEVEL_STATE, !host, 2, NULL, NULL, host_value);
                free(host_value);
                
                // Process port with environment variable support and default
                json_t* port = json_object_get(conn, "Port");
                if (json_is_integer(port)) {
                    log_config_section_item("Port", "%d", LOG_LEVEL_STATE, 0, 2, NULL, NULL, json_integer_value(port));
                } else {
                    char* port_value = get_config_string_with_env("Port", port, "5432"); // Default postgres port
                    log_config_section_item("Port", "%s", LOG_LEVEL_STATE, !port, 2, NULL, NULL, port_value);
                    free(port_value);
                }
                
                // Process database name with environment variable support and default
                json_t* database = json_object_get(conn, "Database");
                char* db_value = get_config_string_with_env("Database", database, conn_names[i]);
                log_config_section_item("Database", "%s", LOG_LEVEL_STATE, !database, 2, NULL, NULL, db_value);
                free(db_value);
                
                // Process username with environment variable support but don't show actual value for security
                json_t* username = json_object_get(conn, "Username");
                char* username_value = get_config_string_with_env("Username", username, "postgres"); // Default postgres username
                // Just log that it's configured, not the actual value
                log_config_section_item("Username", "configured", LOG_LEVEL_STATE, !username, 2, NULL, NULL);
                free(username_value);
                
                // Process password but don't log it at all (security)
                json_t* password = json_object_get(conn, "Password");
                char* password_value = get_config_string_with_env("Password", password, "");
                free(password_value); // Just process and free, don't log
                
                // Enabled state for this database
                json_t* enabled = json_object_get(conn, "Enabled");
                bool db_enabled = get_config_bool(enabled, true);
                log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 2, NULL, NULL,
                    db_enabled ? "true" : "false");
                
                // Workers for this database (use DefaultWorkers if not specified)
                json_t* workers = json_object_get(conn, "Workers");
                if (json_is_integer(workers)) {
                    log_config_section_item("Workers", "%d", LOG_LEVEL_STATE, 0, 2, NULL, NULL, json_integer_value(workers));
                } else {
                    log_config_section_item("Workers", "%d", LOG_LEVEL_STATE, 1, 2, NULL, NULL, default_workers_value);
                }
                
                json_t* max_conn = json_object_get(conn, "MaxConnections");
                if (json_is_integer(max_conn)) {
                    log_config_section_item("MaxConnections", "%d", LOG_LEVEL_STATE, 0, 2, NULL, NULL, json_integer_value(max_conn));
                }
            }
            
            free(conn_names);
        } else {
            // If memory allocation fails, display without sorting
            const char* name;
            json_t* conn;
            json_object_foreach(connections, name, conn) {
                log_config_section_item(name, "configured", LOG_LEVEL_STATE, 0, 1, NULL, NULL);
            }
        }
    }
} else {
    log_config_section_header("Databases");
    log_config_section_item("Status", "Section missing", LOG_LEVEL_ALERT, 1, 0, NULL, NULL);
}