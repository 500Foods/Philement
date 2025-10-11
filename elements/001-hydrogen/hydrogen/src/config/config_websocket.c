/*
 * WebSocket Configuration Implementation
 *
 * Implements the configuration handlers for the WebSocket subsystem,
 * including JSON parsing and environment variable handling.
 */

 // Global includes 
#include <src/hydrogen.h>

// Local includes
#include "config_websocket.h"

// Load WebSocket configuration from JSON
bool load_websocket_config(json_t* root, AppConfig* config) {

    // Initialize with robust defaults that match no-config behavior
    WebSocketConfig* ws = &config->websocket;
    ws->enable_ipv4 = false; 
    ws->enable_ipv6 = false;
    ws->lib_log_level = 2;
    ws->port = 5001;
    ws->max_message_size = 2048;
    
    // Initialize connection timeouts with defaults
    ws->connection_timeouts.shutdown_wait_seconds = 2;
    ws->connection_timeouts.service_loop_delay_ms = 50;
    ws->connection_timeouts.connection_cleanup_ms = 500;
    ws->connection_timeouts.exit_wait_seconds = 3;
    
    // Initialize string fields with defaults
    ws->protocol = strdup("hydrogen");
    if (!ws->protocol) {
        log_this(SR_CONFIG, "Failed to allocate protocol string, using hardcoded default", LOG_LEVEL_ALERT, 0);
        ws->protocol = NULL; // Will be handled by WebSocket code
    }

    ws->key = strdup("${env.WEBSOCKET_KEY}");
    if (!ws->key) {
        log_this(SR_CONFIG, "Failed to allocate key string, using hardcoded default", LOG_LEVEL_ALERT, 0);
        ws->key = NULL; // Will be handled by WebSocket code
    }

    // If no JSON root provided, use defaults (matches no-config behavior)
    if (!root) {
        log_this(SR_CONFIG, "No configuration provided, using defaults", LOG_LEVEL_DEBUG, 0);
        return true;
    }

    // Process main section - if WebSocketServer section doesn't exist, keep defaults
    const json_t* websocket_section = json_object_get(root, "WebSocketServer");
    if (!websocket_section) {
        log_this(SR_CONFIG, "WebSocketServer section not found, using defaults", LOG_LEVEL_DEBUG, 0);
        return true; // Use defaults
    }

    // Process basic settings with fallbacks (never fail)
    (void)PROCESS_BOOL(root, ws, enable_ipv4, "WebSocketServer.EnableIPv4", "WebSocket");
    (void)PROCESS_BOOL(root, ws, enable_ipv6, "WebSocketServer.EnableIPv6", "WebSocket");
    (void)PROCESS_INT(root, ws, lib_log_level, "WebSocketServer.LibLogLevel", "WebSocket");
    (void)PROCESS_INT(root, ws, port, "WebSocketServer.Port", "WebSocket");
    (void)PROCESS_STRING(root, ws, protocol, "WebSocketServer.Protocol", "WebSocket");
    (void)PROCESS_SENSITIVE(root, ws, key, "WebSocketServer.Key", "WebSocket");
    (void)PROCESS_SIZE(root, ws, max_message_size, "WebSocketServer.MaxMessageSize", "WebSocket");
    
    // Process connection timeouts with fallbacks (never fail)
    (void)PROCESS_INT(root, &ws->connection_timeouts, shutdown_wait_seconds, "WebSocketServer.ConnectionTimeouts.ShutdownWaitSeconds", "WebSocket");
    (void)PROCESS_INT(root, &ws->connection_timeouts, service_loop_delay_ms, "WebSocketServer.ConnectionTimeouts.ServiceLoopDelayMs", "WebSocket");
    (void)PROCESS_INT(root, &ws->connection_timeouts, connection_cleanup_ms, "WebSocketServer.ConnectionTimeouts.ConnectionCleanupMs", "WebSocket");
    (void)PROCESS_INT(root, &ws->connection_timeouts, exit_wait_seconds, "WebSocketServer.ConnectionTimeouts.ExitWaitSeconds", "WebSocket");

    log_this(SR_CONFIG, "WebSocket configuration loaded successfully (with fallbacks)", LOG_LEVEL_STATE, 0);
    return true; // Always succeed - use defaults for any missing values
}

// Dump WebSocket configuration
void dump_websocket_config(const WebSocketConfig* config) {
    if (!config) {
        DUMP_TEXT("", "Cannot dump NULL WebSocket config");
        return;
    }

    
    // Dump basic settings
    DUMP_BOOL2("――", "IP4 Enabled", config->enable_ipv4);

    // Dump basic settings
    DUMP_BOOL2("――", "IPv6 Enabled", config->enable_ipv6);
    
    char value_str[128];
    
    // Port
    snprintf(value_str, sizeof(value_str), "Port: %d", config->port);
    DUMP_TEXT("――", value_str);
    
    // Protocol
    snprintf(value_str, sizeof(value_str), "Protocol: %s", 
            config->protocol ? config->protocol : "(not set)");
    DUMP_TEXT("――", value_str);
            
    // Message size with units
    if (config->max_message_size >= 1024 * 1024) {
        snprintf(value_str, sizeof(value_str), "Max Message Size: %zu MB", 
                config->max_message_size / (1024 * 1024));
    } else if (config->max_message_size >= 1024) {
        snprintf(value_str, sizeof(value_str), "Max Message Size: %zu KB", 
                config->max_message_size / 1024);
    } else {
        snprintf(value_str, sizeof(value_str), "Max Message Size: %zu bytes", 
                config->max_message_size);
    }
    DUMP_TEXT("――", value_str);
    
}

// Clean up WebSocket configuration
void cleanup_websocket_config(WebSocketConfig* config) {
    if (!config) {
        return;
    }

    // Free allocated strings
    free(config->protocol);
    free(config->key);

    // Zero out the structure
    memset(config, 0, sizeof(WebSocketConfig));
}
