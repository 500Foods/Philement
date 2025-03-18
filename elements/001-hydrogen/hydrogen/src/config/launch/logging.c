/*
 * Logging Subsystem Launch Readiness Check
 * 
 * This module verifies that all prerequisites for the logging subsystem
 * are satisfied before attempting to initialize it.
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include "launch.h"
#include "../../logging/logging.h"
#include "../../config/config.h"
#include "../../config/config_filesystem.h"

// External declarations
extern AppConfig* app_config;

// Static message array for readiness check results
static const char* logging_messages[10]; // Up to 10 messages plus NULL terminator
static int message_count = 0;

// Add a message to the messages array
static void add_message(const char* format, ...) {
    if (message_count >= 9) return; // Leave room for NULL terminator
    
    va_list args;
    va_start(args, format);
    char* message = NULL;
    vasprintf(&message, format, args);
    va_end(args);
    
    if (message) {
        logging_messages[message_count++] = message;
        logging_messages[message_count] = NULL; // Ensure NULL termination
    }
}

// Check if the logging subsystem is ready to launch
LaunchReadiness check_logging_launch_readiness(void) {
    bool overall_readiness = true;
    message_count = 0;
    
    // Clear messages array
    for (int i = 0; i < 10; i++) {
        logging_messages[i] = NULL;
    }
    
    // Check 1: Configuration loaded
    bool config_loaded = (app_config != NULL);
    if (config_loaded) {
        add_message("Go: Configuration is loaded");
    } else {
        add_message("No-Go: Configuration is not loaded");
        overall_readiness = false;
    }
    
    // Only proceed with other checks if configuration is loaded
    if (config_loaded) {
        // Check 2: Log file directory exists and is writable (if log file is specified)
        if (app_config->server.log_file) {
            char* log_dir = strdup(app_config->server.log_file);
            if (log_dir) {
                // Get the directory part of the path
                char* last_slash = strrchr(log_dir, '/');
                if (last_slash) {
                    *last_slash = '\0'; // Truncate at the last slash to get directory
                    
                    // Check if directory exists and is writable
                    if (access(log_dir, F_OK) == 0 && access(log_dir, W_OK) == 0) {
                        add_message("Go: Log directory '%s' exists and is writable", log_dir);
                    } else {
                        add_message("No-Go: Log directory '%s' does not exist or is not writable", log_dir);
                        overall_readiness = false;
                    }
                }
                free(log_dir);
            }
        } else {
            add_message("Go: No log file specified, using console only");
        }
        
        // Check 3: Logging levels are valid
        bool levels_valid = true;
        if (app_config->logging.console.default_level < 0 || app_config->logging.console.default_level > 5) {
            add_message("No-Go: Invalid console logging level: %d", app_config->logging.console.default_level);
            levels_valid = false;
            overall_readiness = false;
        }
        
        if (app_config->logging.file.default_level < 0 || app_config->logging.file.default_level > 5) {
            add_message("No-Go: Invalid file logging level: %d", app_config->logging.file.default_level);
            levels_valid = false;
            overall_readiness = false;
        }
        
        if (levels_valid) {
            add_message("Go: Logging levels are valid");
        }
    }
    
    // Final decision
    if (overall_readiness) {
        add_message("Decision: Go for Launch");
    } else {
        add_message("Decision: No-Go for Launch");
    }
    
    // Build the readiness structure
    LaunchReadiness readiness = {
        .subsystem = "Logging",
        .ready = overall_readiness,
        .messages = logging_messages
    };
    
    return readiness;
}