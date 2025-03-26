/*
 * Web Server Subsystem Launch Readiness Check
 * 
 * This module verifies that all prerequisites for the web server subsystem
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
extern volatile sig_atomic_t web_server_shutdown;

// Static message array for readiness check results
static const char* webserver_messages[25]; // Up to 25 messages plus NULL terminator
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
        webserver_messages[message_count++] = message;
        webserver_messages[message_count] = NULL; // Ensure NULL termination
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
            webserver_messages[message_count++] = message;
            webserver_messages[message_count] = NULL; // Ensure NULL termination
        }
    }
}

// Check if the web server subsystem is ready to launch
LaunchReadiness check_webserver_launch_readiness(void) {
    bool overall_readiness = true;
    message_count = 0;
    
    // Clear messages array
    for (int i = 0; i < 25; i++) {
        webserver_messages[i] = NULL;
    }
    
    // Add subsystem name as first message
    add_message("WebServer");
    
    // Check if configuration is loaded
    if (app_config == NULL) {
        add_go_message(false, "Configuration not loaded");
        
        // Build the readiness structure with failure
        LaunchReadiness readiness = {
            .subsystem = "WebServer",
            .ready = false,
            .messages = webserver_messages
        };
        
        return readiness;
    }
    
    // Check 1: Enabled in configuration
    bool is_enabled = app_config->web.enabled;
    add_go_message(is_enabled, "Enabled (%s in configuration)", is_enabled ? "enabled" : "disabled");
    if (!is_enabled) {
        overall_readiness = false;
    }
    
    // Check 2: IPv6 enabled
    bool ipv6_enabled = app_config->web.enable_ipv6;
    add_go_message(true, "IPv6 (%s)", ipv6_enabled ? "enabled" : "disabled");
    // Note: IPv6 being disabled is not a No-Go condition, just informational
    
    // Check 3: Port configuration
    // Valid ports: 80, 443, or in range 1024-65546
    int port = app_config->web.port;
    bool valid_port = (port == 80 || port == 443 || (port >= 1024 && port <= 65546));
    add_go_message(valid_port, "Port Configuration (%s: %d)", valid_port ? "valid" : "invalid", port);
    if (!valid_port) {
        overall_readiness = false;
    }
    
    // Check 4: WebRoot not null
    bool web_root_valid = (app_config->web.web_root != NULL && strlen(app_config->web.web_root) > 0);
    add_go_message(web_root_valid, "WebRoot (%s)", web_root_valid ? app_config->web.web_root : "not set");
    if (!web_root_valid) {
        overall_readiness = false;
    }
    
    // Check 5: UploadPath not null
    bool upload_path_valid = (app_config->web.upload_path != NULL && strlen(app_config->web.upload_path) > 0);
    add_go_message(upload_path_valid, "UploadPath (%s)", upload_path_valid ? app_config->web.upload_path : "not set");
    if (!upload_path_valid) {
        overall_readiness = false;
    }
    
    // Check 6: UploadDir not null
    bool upload_dir_valid = (app_config->web.upload_dir != NULL && strlen(app_config->web.upload_dir) > 0);
    add_go_message(upload_dir_valid, "UploadDir (%s)", upload_dir_valid ? app_config->web.upload_dir : "not set");
    if (!upload_dir_valid) {
        overall_readiness = false;
    }
    
    // Check 7: MaxUploadSize not null (size_t is unsigned, so we just check if it's > 0)
    bool max_upload_size_valid = (app_config->web.max_upload_size > 0);
    add_go_message(max_upload_size_valid, "MaxUploadSize (%zu bytes)", app_config->web.max_upload_size);
    if (!max_upload_size_valid) {
        overall_readiness = false;
    }
    
    // Check 8: Not in shutdown state
    bool not_in_shutdown = !web_server_shutdown;
    add_go_message(not_in_shutdown, "Shutdown State (%s)", not_in_shutdown ? "not in shutdown" : "in shutdown");
    if (!not_in_shutdown) {
        overall_readiness = false;
    }
    
    // Check 9: Network subsystem registered
    int network_id = get_subsystem_id_by_name("Network");
    bool network_registered = (network_id >= 0);
    add_go_message(network_registered, "Network dependency (subsystem %s)", network_registered ? "registered" : "not registered");
    if (!network_registered) {
        overall_readiness = false;
    }
    
    // Final decision
    if (overall_readiness) {
        add_message("  Decide:  Go For Launch of WebServer Subsystem");
    } else {
        add_message("  Decide:  No-Go For Launch of WebServer Subsystem");
    }
    
    // Build the readiness structure
    LaunchReadiness readiness = {
        .subsystem = "WebServer",
        .ready = overall_readiness,
        .messages = webserver_messages
    };
    
    return readiness;
}