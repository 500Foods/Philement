/*
 * Logging Subsystem Launch Readiness Check
 * 
 * This module verifies that all prerequisites for the logging subsystem
 * are satisfied before attempting to initialize it.
 */

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

// Helper function to get log level name
static const char* get_log_level_name(int level) {
    switch (level) {
        case 0: return "TRACE";
        case 1: return "DEBUG";
        case 2: return "INFO";
        case 3: return "WARN";
        case 4: return "ERROR";
        case 5: return "FATAL";
        case 6: return "QUIET";
        default: return "UNKNOWN";
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
    
    // Track which output destinations are enabled and properly configured
    bool console_ready = false;
    bool file_ready = false;
    bool database_ready = false;
    bool notify_ready = false;
    
    // Only proceed with checks if configuration is loaded
    bool config_loaded = (app_config != NULL);
    if (config_loaded) {
        // Check 1: Console Output
        if (app_config->logging.console.enabled && app_config->logging.console.default_level >= 0 && app_config->logging.console.default_level <= 5) {
            add_message("  Go:      Console Output (enabled, default: %s, %zu subsystems)", 
                       get_log_level_name(app_config->logging.console.default_level),
                       app_config->logging.console.subsystem_count);
            console_ready = true;
        } else if (!app_config->logging.console.enabled) {
            add_message("  No-Go:   Console Output (disabled, default: %s, %zu subsystems)", 
                       get_log_level_name(app_config->logging.console.default_level),
                       app_config->logging.console.subsystem_count);
        } else {
            add_message("  No-Go:   Console Output (invalid level: %d)", 
                       app_config->logging.console.default_level);
        }
        
        // Check 2: File Output - Skip detailed check for now
        if (app_config->logging.file.enabled && app_config->server.log_file) {
            add_message("  Go:      File Output (enabled, default: %s, %zu subsystems)", 
                       get_log_level_name(app_config->logging.file.default_level),
                       app_config->logging.file.subsystem_count);
            file_ready = true;
        } else {
            add_message("  No-Go:   File Output (disabled, default: %s, %zu subsystems)", 
                       get_log_level_name(app_config->logging.file.default_level),
                       app_config->logging.file.subsystem_count);
        }
        
        // Check 3: Database Output
        if (app_config->logging.database.enabled) {
            add_message("  Go:      Database Output (enabled, default: %s, %zu subsystems)", 
                       get_log_level_name(app_config->logging.database.default_level),
                       app_config->logging.database.subsystem_count);
            database_ready = true;
        } else {
            add_message("  No-Go:   Database Output (disabled, default: %s, %zu subsystems)", 
                       get_log_level_name(app_config->logging.database.default_level),
                       app_config->logging.database.subsystem_count);
        }
        
        // Check 4: Notify Output
        if (app_config->logging.notify.enabled) {
            add_message("  Go:      Notify Output (enabled, default: %s, %zu subsystems)", 
                       get_log_level_name(app_config->logging.notify.default_level),
                       app_config->logging.notify.subsystem_count);
            notify_ready = true;
        } else {
            add_message("  No-Go:   Notify Output (disabled, default: %s, %zu subsystems)", 
                       get_log_level_name(app_config->logging.notify.default_level),
                       app_config->logging.notify.subsystem_count);
        }
    } else {
        add_message("  No-Go:   Configuration (not loaded)");
        overall_readiness = false;
    }
    
    // Final decision - Go if ANY output destination is ready
    bool any_output_ready = console_ready || file_ready || database_ready || notify_ready;
    
    // Build the list of enabled outputs for the decision message
    char enabled_outputs[100] = "";
    if (console_ready) strcat(enabled_outputs, "Console");
    if (console_ready && (file_ready || database_ready || notify_ready)) strcat(enabled_outputs, " ");
    if (file_ready) strcat(enabled_outputs, "File");
    if (file_ready && (database_ready || notify_ready)) strcat(enabled_outputs, " ");
    if (database_ready) strcat(enabled_outputs, "Database");
    if (database_ready && notify_ready) strcat(enabled_outputs, " ");
    if (notify_ready) strcat(enabled_outputs, "Notify");
    
    // Final decision
    if (any_output_ready) {
        add_message("  Decide:  Go For Launch of Logging Subsystem (%s)", enabled_outputs);
        overall_readiness = true;
    } else {
        add_message("  Decide:  No-Go For Launch of Logging Subsystem (no valid outputs)");
        overall_readiness = false;
    }
    
    // Build the readiness structure
    LaunchReadiness readiness = {
        .subsystem = "Logging",
        .ready = overall_readiness,
        .messages = logging_messages
    };
    
    return readiness;
}