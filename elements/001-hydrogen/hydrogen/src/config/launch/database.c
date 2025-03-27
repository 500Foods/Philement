/*
 * Database Subsystem Launch Readiness Check
 * 
 * This module verifies that all prerequisites for the database subsystem
 * are satisfied before attempting to initialize it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <jansson.h>

#include "launch.h"
#include "../../logging/logging.h"
#include "../../config/config.h"
#include "../../state/registry/subsystem_registry.h"

// External declarations
extern AppConfig* app_config;

// Static message array for readiness check results
static const char* database_messages[25]; // Up to 25 messages plus NULL terminator
static int message_count = 0;

// Add a message to the messages array
static void add_message(const char* format, ...) {
    if (message_count >= 24) return; // Leave room for NULL terminator
    
    va_list args;
    va_start(args, format);
    char* message = NULL;
    vasprintf(&message, format, args);
    va_end(args);
    
    if (message) {
        database_messages[message_count++] = message;
        database_messages[message_count] = NULL; // Ensure NULL termination
    }
}

// Helper function to add a Go/No-Go message with proper formatting
static void add_go_message(bool is_go, const char* format, ...) {
    if (message_count >= 24) return; // Leave room for NULL terminator
    
    // Format the message content
    va_list args;
    va_start(args, format);
    char* content = NULL;
    vasprintf(&content, format, args);
    va_end(args);
    
    if (content) {
        // Format the full message with Go/No-Go prefix
        char* message = NULL;
        if (is_go) {
            asprintf(&message, "  Go:      %s", content);
        } else {
            asprintf(&message, "  No-Go:   %s", content);
        }
        
        free(content); // Free the content string
        
        if (message) {
            database_messages[message_count++] = message;
            database_messages[message_count] = NULL; // Ensure NULL termination
        }
    }
}

// Check if the database subsystem is ready to launch
LaunchReadiness check_database_launch_readiness(void) {
    bool overall_readiness = false;
    int databases_go_count = 0;
    message_count = 0;
    
    // Clear messages array
    for (int i = 0; i < 25; i++) {
        database_messages[i] = NULL;
    }
    
    // Add subsystem name as first message
    add_message("Database");
    
    // Check if configuration is loaded
    if (app_config == NULL) {
        add_go_message(false, "Configuration not loaded");
        
        // Build the readiness structure with failure
        LaunchReadiness readiness = {
            .subsystem = "Database",
            .ready = false,
            .messages = database_messages
        };
        
        return readiness;
    }
    
    // Get the JSON root object
    json_t* root = json_load_file(app_config->server.config_file, 0, NULL);
    if (!root) {
        add_go_message(false, "Failed to load configuration file");
        
        // Build the readiness structure with failure
        LaunchReadiness readiness = {
            .subsystem = "Database",
            .ready = false,
            .messages = database_messages
        };
        
        return readiness;
    }
    
    // Get the Databases section
    json_t* databases = json_object_get(root, "Databases");
    if (!json_is_object(databases)) {
        add_go_message(false, "Databases section missing or invalid");
        json_decref(root);
        
        // Build the readiness structure with failure
        LaunchReadiness readiness = {
            .subsystem = "Database",
            .ready = false,
            .messages = database_messages
        };
        
        return readiness;
    }
    
    // Get the DefaultWorkers value
    json_t* default_workers = json_object_get(databases, "DefaultWorkers");
    int default_workers_value = 1;  // Default value
    if (json_is_integer(default_workers)) {
        default_workers_value = json_integer_value(default_workers);
    }
    
    // Get the Connections section
    json_t* connections = json_object_get(databases, "Connections");
    if (!json_is_object(connections)) {
        add_go_message(false, "Connections section missing or invalid");
        json_decref(root);
        
        // Build the readiness structure with failure
        LaunchReadiness readiness = {
            .subsystem = "Database",
            .ready = false,
            .messages = database_messages
        };
        
        return readiness;
    }
    
    // Check each database connection
    const char* db_name;
    json_t* db_conn;
    json_object_foreach(connections, db_name, db_conn) {
        // Check if the database is enabled
        json_t* enabled = json_object_get(db_conn, "Enabled");
        bool db_enabled = true;  // Default to true if not specified
        if (json_is_boolean(enabled)) {
            db_enabled = json_boolean_value(enabled);
        }
        
        // Get the database type
        json_t* type = json_object_get(db_conn, "Type");
        const char* db_type = "unknown";
        if (json_is_string(type)) {
            db_type = json_string_value(type);
        }
        
        // Get the number of workers
        json_t* workers = json_object_get(db_conn, "Workers");
        int workers_value = default_workers_value;  // Default to DefaultWorkers if not specified
        if (json_is_integer(workers)) {
            workers_value = json_integer_value(workers);
        }
        
        // Add a Go/No-Go message for this database
        add_go_message(db_enabled, "%s (%s, Workers: %d, Type: %s)", 
                      db_name, 
                      db_enabled ? "enabled" : "disabled", 
                      workers_value, 
                      db_type);
        
        // Count the number of enabled databases
        if (db_enabled) {
            databases_go_count++;
        }
    }
    
    // Final decision - Go if at least one database is enabled
    overall_readiness = (databases_go_count > 0);
    
    // Add the Decide message
    if (overall_readiness) {
        add_message("  Decide:  Go For Launch of Database Subsystem (%d of 5 databases enabled)", databases_go_count);
    } else {
        add_message("  Decide:  No-Go For Launch of Database Subsystem (no databases enabled)");
    }
    
    // Clean up
    json_decref(root);
    
    // Build the readiness structure
    LaunchReadiness readiness = {
        .subsystem = "Database",
        .ready = overall_readiness,
        .messages = database_messages
    };
    
    return readiness;
}