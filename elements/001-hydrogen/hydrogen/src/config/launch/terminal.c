/*
 * Terminal Subsystem Launch Readiness Check
 * 
 * This module verifies that all prerequisites for the terminal subsystem
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
static const char* terminal_messages[15]; // Up to 15 messages plus NULL terminator
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
        terminal_messages[message_count++] = message;
        terminal_messages[message_count] = NULL; // Ensure NULL termination
    }
}

// Check if the terminal subsystem is ready to launch
LaunchReadiness check_terminal_launch_readiness(void) {
    bool overall_readiness = false;  // Always set to false for No-Go status
    message_count = 0;
    
    // Clear messages array
    for (int i = 0; i < 15; i++) {
        terminal_messages[i] = NULL;
    }
    
    // Add subsystem name as first message
    add_message("Terminal");
    
    // Check 1: Enabled
    add_message("  No-Go:   Enabled (check)");
    
    // Check 2: Configuration loaded
    bool config_loaded = (app_config != NULL);
    if (config_loaded) {
        add_message("  Go:      Configuration (loaded)");
    } else {
        add_message("  No-Go:   Configuration (not loaded)");
    }
    
    // Check 3: Payload
    add_message("  No-Go:   Payload (found)");
    
    // Check 4: WebServer Dependency
    add_message("  No-Go:   WebServer (dependency check)");
    
    // Check 5: WebSockets Dependency
    add_message("  No-Go:   WebSockets (dependency check)");
    
    // Final decision - always No-Go for now
    add_message("  Decide:  No-Go For Launch of Terminal Subsystem");
    
    // Build the readiness structure
    LaunchReadiness readiness = {
        .subsystem = "Terminal",
        .ready = overall_readiness,
        .messages = terminal_messages
    };
    
    return readiness;
}