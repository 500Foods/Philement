/*
 * Launch Terminal Subsystem
 * 
 * This module handles the initialization of the terminal subsystem.
 * It provides functions for checking readiness and launching the terminal.
 * 
 * Dependencies:
 * - WebServer subsystem must be initialized and ready
 * - WebSockets subsystem must be initialized and ready
 */

// Global includes 
#include "../hydrogen.h"

// Local includes
#include "launch.h"

// Check if the terminal subsystem is ready to launch
LaunchReadiness check_terminal_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool is_ready = true;

    const AppConfig* config = get_app_config();

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup("Terminal"));

    // Check dependencies first
    if (!config->webserver.enable_ipv4 && !config->webserver.enable_ipv6) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   WebServer Not Enabled"));
        add_launch_message(&messages, &count, &capacity, strdup("  Reason:  Terminal Requires WebServer (IPv4 or IPv6)"));
        is_ready = false;
    }

    if (!config->websocket.enabled) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   WebSocket Not Enabled"));
        add_launch_message(&messages, &count, &capacity, strdup("  Reason:  Terminal Requires WebSocket"));
        is_ready = false;
    }

    // Check if terminal is enabled
    if (!config->terminal.enabled) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Terminal System Disabled"));
        add_launch_message(&messages, &count, &capacity, strdup("  Reason:  Disabled in Configuration"));
        is_ready = false;
    } else {
        // Validate required strings
        if (!config->terminal.web_path) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Missing Web Path"));
            add_launch_message(&messages, &count, &capacity, strdup("  Reason:  Web Path Must Be Set"));
            is_ready = false;
        }

        if (!config->terminal.shell_command) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Missing Shell Command"));
            add_launch_message(&messages, &count, &capacity, strdup("  Reason:  Shell Command Must Be Set"));
            is_ready = false;
        }

        // Validate numeric ranges
        if (config->terminal.max_sessions < 1 || config->terminal.max_sessions > 100) {
            char msg[128];
            snprintf(msg, sizeof(msg), "  No-Go:   Invalid Max Sessions: %d",
                    config->terminal.max_sessions);
            add_launch_message(&messages, &count, &capacity, strdup(msg));
            add_launch_message(&messages, &count, &capacity, strdup("  Reason:  Must Be Between 1 and 100"));
            is_ready = false;
        }

        if (config->terminal.idle_timeout_seconds < 60 ||
            config->terminal.idle_timeout_seconds > 3600) {
            char msg[128];
            snprintf(msg, sizeof(msg), "  No-Go:   Invalid Idle Timeout: %d",
                    config->terminal.idle_timeout_seconds);
            add_launch_message(&messages, &count, &capacity, strdup(msg));
            add_launch_message(&messages, &count, &capacity, strdup("  Reason:  Must Be Between 60 and 3600 Seconds"));
            is_ready = false;
        }
    }

    // Add final decision message
    if (is_ready) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Terminal System Ready"));
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of Terminal"));
    }

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = "Terminal",
        .ready = is_ready,
        .messages = messages
    };
}

// Launch the terminal subsystem
int launch_terminal_subsystem(void) {
    // Reset shutdown flag
    terminal_system_shutdown = 0;
    
    // Get configuration
    const AppConfig* config = get_app_config();
    if (!config || !config->terminal.enabled) {
        return -1;
    }
    
    // Initialize terminal system
    log_this("Terminal", "Initializing terminal subsystem", LOG_LEVEL_STATE);
    
    return 0;
}
