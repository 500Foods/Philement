/*
 * WebSocket Configuration Implementation
 *
 * Implements the configuration handlers for the WebSocket subsystem,
 * including JSON parsing and environment variable handling.
 */

#include <stdlib.h>
#include <string.h>
#include "config_websocket.h"
#include "config.h"
#include "config_utils.h"
#include "../logging/logging.h"

// Load WebSocket configuration from JSON
bool load_websocket_config(json_t* root, AppConfig* config) {

    // Initialize with defaults
    WebSocketConfig* ws = &config->websocket;
    ws->enabled = true;
    ws->enable_ipv6 = false;
    ws->port = 5001;
    ws->max_message_size = 1024 * 1024;  // 1MB
    ws->exit_wait_seconds = 5;
    
    // Initialize string fields with defaults
    ws->protocol = strdup("hydrogen");
    if (!ws->protocol) {
        log_this("Config-WebSocket", "Failed to allocate protocol string", LOG_LEVEL_ERROR);
        return false;
    }

    ws->key = strdup("${env.WEBSOCKET_KEY}");
    if (!ws->key) {
        log_this("Config-WebSocket", "Failed to allocate key string", LOG_LEVEL_ERROR);
        free(ws->protocol);
        ws->protocol = NULL;
        return false;
    }

    // Process configuration values
    bool success = true;

    // Process main section and enabled flag
    success = PROCESS_SECTION(root, "WebSocket");
    success = success && PROCESS_BOOL(root, ws, enabled, "WebSocket.Enabled", "WebSocket");
    
    if (ws->enabled) {
        // Process basic settings
        success = success && PROCESS_BOOL(root, ws, enable_ipv6, "WebSocket.EnableIPv6", "WebSocket");
        success = success && PROCESS_INT(root, ws, port, "WebSocket.Port", "WebSocket");
        success = success && PROCESS_STRING(root, ws, protocol, "WebSocket.Protocol", "WebSocket");
        success = success && PROCESS_SENSITIVE(root, ws, key, "WebSocket.Key", "WebSocket");
        success = success && PROCESS_SIZE(root, ws, max_message_size, "WebSocket.MaxMessageSize", "WebSocket");

    }

    return success;
}

// Dump WebSocket configuration
void dump_websocket_config(const WebSocketConfig* config) {
    if (!config) {
        DUMP_TEXT("", "Cannot dump NULL WebSocket config");
        return;
    }

    // Dump enabled status
    DUMP_BOOL2("――", "Enabled", config->enabled);
    
    if (config->enabled) {
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