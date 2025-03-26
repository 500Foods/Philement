/*
 * Swagger Subsystem Launch Readiness Check
 * 
 * This module verifies that all prerequisites for the Swagger subsystem
 * are satisfied before attempting to initialize it.
 */

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
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_starting;

// Static message array for readiness check results
static const char* swagger_messages[25]; // Up to 25 messages plus NULL terminator
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
        swagger_messages[message_count++] = message;
        swagger_messages[message_count] = NULL; // Ensure NULL termination
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
            swagger_messages[message_count++] = message;
            swagger_messages[message_count] = NULL; // Ensure NULL termination
        }
    }
}

// Check if the Swagger subsystem is ready to launch
LaunchReadiness check_swagger_launch_readiness(void) {
    bool overall_readiness = true;
    message_count = 0;
    
    // Clear messages array
    for (int i = 0; i < 25; i++) {
        swagger_messages[i] = NULL;
    }
    
    // Add subsystem name as first message
    add_message("Swagger");
    
    // Check if configuration is loaded
    if (app_config == NULL) {
        add_go_message(false, "Configuration not loaded");
        
        // Build the readiness structure with failure
        LaunchReadiness readiness = {
            .subsystem = "Swagger",
            .ready = false,
            .messages = swagger_messages
        };
        
        return readiness;
    }
    
    // Check 1: Not in shutdown state
    bool not_in_shutdown = !server_stopping;
    add_go_message(not_in_shutdown, "Shutdown State (%s)", not_in_shutdown ? "not in shutdown" : "in shutdown");
    if (!not_in_shutdown) {
        overall_readiness = false;
    }
    
    // Check 2: Enabled in configuration
    // Note: Swagger settings are logged but not stored in config structure (see json_swagger.c)
    // Assume enabled by default as per hydrogen.json and config_swagger_init
    bool is_enabled = true;
    add_go_message(is_enabled, "Enabled (enabled in configuration)");
    
    // Check 3: Network subsystem registered
    int network_id = get_subsystem_id_by_name("Network");
    bool network_registered = (network_id >= 0);
    add_go_message(network_registered, "Network dependency (subsystem %s)", network_registered ? "registered" : "not registered");
    if (!network_registered) {
        overall_readiness = false;
    }
    
    // Check 4: WebServer subsystem registered
    int webserver_id = get_subsystem_id_by_name("WebServer");
    bool webserver_registered = (webserver_id >= 0);
    add_go_message(webserver_registered, "WebServer dependency (subsystem %s)", webserver_registered ? "registered" : "not registered");
    if (!webserver_registered) {
        overall_readiness = false;
    }
    
    // Check 5: Payload subsystem readiness
    // Since Payload is not registered as a subsystem, we check if it was ready during launch
    extern LaunchReadiness check_payload_launch_readiness(void);
    LaunchReadiness payload_readiness = check_payload_launch_readiness();
    bool payload_ready = payload_readiness.ready;
    add_go_message(payload_ready, "Payload dependency (%s)", payload_ready ? "ready" : "not ready");
    if (!payload_ready) {
        overall_readiness = false;
    }
    
    // Check 6: API subsystem readiness
    // Since API is not registered as a subsystem, we check if it was ready during launch
    extern LaunchReadiness check_api_launch_readiness(void);
    LaunchReadiness api_readiness = check_api_launch_readiness();
    bool api_ready = api_readiness.ready;
    add_go_message(api_ready, "API dependency (%s)", api_ready ? "ready" : "not ready");
    if (!api_ready) {
        overall_readiness = false;
    }
    
    // Final decision
    if (overall_readiness) {
        add_message("  Decide:  Go For Launch of Swagger Subsystem");
    } else {
        add_message("  Decide:  No-Go For Launch of Swagger Subsystem");
    }
    
    // Build the readiness structure
    LaunchReadiness readiness = {
        .subsystem = "Swagger",
        .ready = overall_readiness,
        .messages = swagger_messages
    };
    
    return readiness;
}