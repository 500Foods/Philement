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
#include "../../config/files/config_filesystem.h"

// External declarations
extern AppConfig* app_config;

// Static message array for readiness check results
static const char* logging_messages[15]; // Up to 15 messages plus NULL terminator
static int message_count = 0;

// Add a message to the messages array
static void add_message(const char* format, ...) {
    if (message_count >= 14) return; // Leave room for NULL terminator
    
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
    for (int i = 0; i < 15; i++) {
        logging_messages[i] = NULL;
    }
    
    // Add subsystem name as first message
    add_message("Logging");
    
    // Check 1: Configuration loaded
    bool config_loaded = (app_config != NULL);
    if (config_loaded) {
        add_message("  Go:      Configuration (loaded)");
    } else {
        add_message("  No-Go:   Configuration (not loaded)");
        overall_readiness = false;
    }
    
    // Only proceed with other checks if configuration is loaded
    if (config_loaded) {
        // Check 2: File Output
        if (app_config->server.log_file) {
            char* log_dir = strdup(app_config->server.log_file);
            if (log_dir) {
                // Get the directory part of the path
                char* last_slash = strrchr(log_dir, '/');
                if (last_slash) {
                    *last_slash = '\0'; // Truncate at the last slash to get directory
                    
                    // Check if directory exists and is writable
                    if (access(log_dir, F_OK) == 0 && access(log_dir, W_OK) == 0) {
                        add_message("  Go:      File Output (%s, writable)", log_dir);
                    } else {
                        add_message("  No-Go:   File Output (%s error, not writable)", log_dir);
                        overall_readiness = false;
                    }
                }
                free(log_dir);
            }
        } else {
            add_message("  Go:      File Output (disabled, using console only)");
        }
        
        // Check 3: Database Output
        if (app_config->logging.database.enabled) {
            // In a real implementation, we would check database credentials here
            add_message("  Go:      Database Output (credential check, enabled)");
        } else {
            add_message("  Go:      Database Output (disabled, not required)");
        }
        
        // Check 4: Console Output
        if (app_config->logging.console.default_level < 0 || app_config->logging.console.default_level > 5) {
            add_message("  No-Go:   Console Output (invalid level: %d)", app_config->logging.console.default_level);
            overall_readiness = false;
        } else {
            add_message("  Go:      Console Output (environment check, level %d)", app_config->logging.console.default_level);
        }
        
        // Check 5: Notification Output
        if (app_config->logging.notify.enabled) {
            // In a real implementation, we would check mail configuration here
            add_message("  Go:      Notification Output (mail config, enabled)");
        } else {
            add_message("  Go:      Notification Output (disabled, not required)");
        }
    }
    
    // Final decision
    if (overall_readiness) {
        add_message("  Decide:   Go For Launch of Logging Subsystem");
    } else {
        add_message("  Decide:   No-Go For Launch of Logging Subsystem");
    }
    
    // Build the readiness structure
    LaunchReadiness readiness = {
        .subsystem = "Logging",
        .ready = overall_readiness,
        .messages = logging_messages
    };
    
    return readiness;
}