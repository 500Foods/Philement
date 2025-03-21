/*
 * Print Subsystem Launch Readiness Check
 * 
 * This module verifies that all prerequisites for the print queue subsystem
 * are satisfied before attempting to initialize it.
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "launch.h"
#include "../../logging/logging.h"
#include "../../config/config.h"
#include "../../state/registry/subsystem_registry.h"

// External declarations
extern AppConfig* app_config;
extern volatile sig_atomic_t print_system_shutdown;
extern int queue_system_initialized;

// Static message array for readiness check results
static const char* print_messages[15]; // Up to 15 messages plus NULL terminator
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
        print_messages[message_count++] = message;
        print_messages[message_count] = NULL; // Ensure NULL termination
    }
}

// Check if the print subsystem is ready to launch
LaunchReadiness check_print_launch_readiness(void) {
    bool overall_readiness = true;
    message_count = 0;
    
    // Clear messages array
    for (int i = 0; i < 15; i++) {
        print_messages[i] = NULL;
    }
    
    // Add subsystem name as first message
    add_message("PrintQueue");
    
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
        // Check 2: Enabled in configuration
        if (app_config->print_queue.enabled) {
            add_message("  Go:      Enabled (in configuration)");
        } else {
            add_message("  No-Go:   Enabled (disabled in configuration)");
            overall_readiness = false;
        }
        
        // Check 3: Not in shutdown state
        if (!print_system_shutdown) {
            add_message("  Go:      Shutdown State (not in shutdown)");
        } else {
            add_message("  No-Go:   Shutdown State (in shutdown)");
            overall_readiness = false;
        }
        
        // Check 4: Queue system initialized
        if (queue_system_initialized) {
            add_message("  Go:      Queue System (initialized)");
        } else {
            add_message("  No-Go:   Queue System (not initialized)");
            overall_readiness = false;
        }
        
        // Check 5: Logging subsystem dependency
        if (is_subsystem_running_by_name("Logging")) {
            add_message("  Go:      Dependency (Logging subsystem running)");
        } else {
            add_message("  No-Go:   Dependency (Logging subsystem not running)");
            overall_readiness = false;
        }
        
        // Check 6: Print buffer configuration
        if (app_config->print_queue.buffers.command_buffer_size > 0) {
            add_message("  Go:      Buffer Configuration (valid: %zu bytes)", 
                      app_config->print_queue.buffers.command_buffer_size);
        } else {
            add_message("  No-Go:   Buffer Configuration (invalid: %zu bytes)", 
                      app_config->print_queue.buffers.command_buffer_size);
            overall_readiness = false;
        }
    }
    
    // Final decision
    if (overall_readiness) {
        add_message("  Decide:  Go For Launch of Print Queue Subsystem");
    } else {
        add_message("  Decide:  No-Go For Launch of Print Queue Subsystem");
    }
    
    // Build the readiness structure
    LaunchReadiness readiness = {
        .subsystem = "PrintQueue",
        .ready = overall_readiness,
        .messages = print_messages
    };
    
    return readiness;
}