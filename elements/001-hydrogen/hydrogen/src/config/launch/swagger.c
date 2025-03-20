/*
 * Swagger Subsystem Launch Readiness Check
 * 
 * This module verifies that all prerequisites for the Swagger subsystem
 * are satisfied before attempting to initialize it.
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "launch.h"
#include "../../logging/logging.h"
#include "../../config/config.h"

// External declarations
extern AppConfig* app_config;

// Static message array for readiness check results
static const char* swagger_messages[15]; // Up to 15 messages plus NULL terminator
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
        swagger_messages[message_count++] = message;
        swagger_messages[message_count] = NULL; // Ensure NULL termination
    }
}

// Check if the Swagger subsystem is ready to launch
LaunchReadiness check_swagger_launch_readiness(void) {
    bool overall_readiness = false;  // Always set to false for No-Go status
    message_count = 0;
    
    // Clear messages array
    for (int i = 0; i < 15; i++) {
        swagger_messages[i] = NULL;
    }
    
    // Add subsystem name as first message
    add_message("Swagger");
    
    // Check 1: Enabled
    add_message("  No-Go:   Enabled (check)");
    
    // Check 2: Configuration loaded
    bool config_loaded = (app_config != NULL);
    if (config_loaded) {
        add_message("  Go:      Configuration (loaded)");
    } else {
        add_message("  No-Go:   Configuration (not loaded)");
    }
    
    // Check 3: API Documentation
    add_message("  No-Go:   API Documentation (payload check)");
    
    // Check 4: WebServer Dependency
    add_message("  No-Go:   WebServer (dependency check)");
    
    // Check 5: Logging Dependency
    add_message("  Go:      Logging (dependency check)");
    
    // Final decision - always No-Go for now
    add_message("  Decide:  No-Go For Launch of Swagger Subsystem");
    
    // Build the readiness structure
    LaunchReadiness readiness = {
        .subsystem = "Swagger",
        .ready = overall_readiness,
        .messages = swagger_messages
    };
    
    return readiness;
}