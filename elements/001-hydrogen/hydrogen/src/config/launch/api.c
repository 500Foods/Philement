/*
 * API Subsystem Launch Readiness Check
 * 
 * This module verifies that all prerequisites for the API subsystem
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
static const char* api_messages[25]; // Up to 25 messages plus NULL terminator
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
        api_messages[message_count++] = message;
        api_messages[message_count] = NULL; // Ensure NULL termination
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
            api_messages[message_count++] = message;
            api_messages[message_count] = NULL; // Ensure NULL termination
        }
    }
}

// Check if the API subsystem is ready to launch
LaunchReadiness check_api_launch_readiness(void) {
    bool overall_readiness = true;
    message_count = 0;
    
    // Clear messages array
    for (int i = 0; i < 25; i++) {
        api_messages[i] = NULL;
    }
    
    // Add subsystem name as first message
    add_message("API");
    
    // Check if configuration is loaded
    if (app_config == NULL) {
        add_go_message(false, "Configuration not loaded");
        
        // Build the readiness structure with failure
        LaunchReadiness readiness = {
            .subsystem = "API",
            .ready = false,
            .messages = api_messages
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
    // API is assumed to be enabled by default unless explicitly disabled
    // The enabled flag is not stored in the APIConfig structure, but we can check
    // if the API prefix is set in the WebServer configuration
    bool is_enabled = (app_config->web.api_prefix != NULL && strlen(app_config->web.api_prefix) > 0);
    
    add_go_message(is_enabled, "Enabled (%s in configuration)", is_enabled ? "enabled" : "disabled");
    if (!is_enabled) {
        overall_readiness = false;
    }
    
    // Check 3: JWTSecret has a not-null value
    bool jwt_secret_valid = (app_config->api.jwt_secret != NULL && strlen(app_config->api.jwt_secret) > 0);
    add_go_message(jwt_secret_valid, "JWT Secret (%s)", jwt_secret_valid ? "configured" : "not configured");
    if (!jwt_secret_valid) {
        overall_readiness = false;
    }
    
    // Check 4: Network subsystem registered
    int network_id = get_subsystem_id_by_name("Network");
    bool network_registered = (network_id >= 0);
    add_go_message(network_registered, "Network dependency (subsystem %s)", network_registered ? "registered" : "not registered");
    if (!network_registered) {
        overall_readiness = false;
    }
    
    // Check 5: WebServer subsystem registered
    int webserver_id = get_subsystem_id_by_name("WebServer");
    bool webserver_registered = (webserver_id >= 0);
    add_go_message(webserver_registered, "WebServer dependency (subsystem %s)", webserver_registered ? "registered" : "not registered");
    if (!webserver_registered) {
        overall_readiness = false;
    }
    
    // Final decision
    if (overall_readiness) {
        add_message("  Decide:  Go For Launch of API Subsystem");
    } else {
        add_message("  Decide:  No-Go For Launch of API Subsystem");
    }
    
    // Build the readiness structure
    LaunchReadiness readiness = {
        .subsystem = "API",
        .ready = overall_readiness,
        .messages = api_messages
    };
    
    return readiness;
}