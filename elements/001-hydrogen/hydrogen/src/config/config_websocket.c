/*
 * WebSocket Configuration Implementation
 *
 * Implements the configuration handlers for the WebSocket subsystem,
 * including JSON parsing, environment variable handling, and validation.
 */

#include <stdlib.h>
#include <string.h>
#include "config_websocket.h"
#include "config.h"
#include "config_utils.h"
#include "../logging/logging.h"

// Validation limits
#define MIN_PORT 1024
#define MAX_PORT 65535
#define MIN_EXIT_WAIT_SECONDS 1
#define MAX_EXIT_WAIT_SECONDS 60
#define WEBSOCKET_MIN_MESSAGE_SIZE 1024        // 1KB
#define WEBSOCKET_MAX_MESSAGE_SIZE 0x40000000  // 1GB

// Function prototypes for internal functions
static int validate_protocol(const char* protocol);
static int validate_key(const char* key);

// Load WebSocket configuration from JSON
bool load_websocket_config(json_t* root, AppConfig* config) {
    // Initialize with defaults
    config->websocket = (WebSocketConfig){
        .enabled = true,
        .enable_ipv6 = false,
        .port = 5001,
        .protocol = "hydrogen",
        .key = "${env.WEBSOCKET_KEY}",
        .max_message_size = 1024 * 1024,  // 1MB
        .exit_wait_seconds = 5
    };

    // Process all config items in sequence
    bool success = true;

    // One line per key/value using macros
    success = PROCESS_SECTION(root, "WebSocketServer");
    success = success && PROCESS_BOOL(root, &config->websocket, enabled, "WebSocketServer.Enabled", "WebSocketServer");
    
    if (config->websocket.enabled) {
        success = success && PROCESS_BOOL(root, &config->websocket, enable_ipv6, "WebSocketServer.EnableIPv6", "WebSocketServer");
        success = success && PROCESS_INT(root, &config->websocket, port, "WebSocketServer.Port", "WebSocketServer");
        success = success && PROCESS_STRING(root, &config->websocket, protocol, "WebSocketServer.Protocol", "WebSocketServer");
        success = success && PROCESS_SENSITIVE(root, &config->websocket, key, "WebSocketServer.Key", "WebSocketServer");
        success = success && PROCESS_SIZE(root, &config->websocket, max_message_size, "WebSocketServer.MaxMessageSize", "WebSocketServer");
        
        // Handle nested ConnectionTimeouts section
        json_t* timeouts = json_object_get(json_object_get(root, "WebSocketServer"), "ConnectionTimeouts");
        if (json_is_object(timeouts)) {
            success = success && PROCESS_INT(timeouts, &config->websocket, exit_wait_seconds, "ExitWaitSeconds", "WebSocketServer.ConnectionTimeouts");
        }

        // Check for legacy protocol key
        if (!json_object_get(json_object_get(root, "WebSocketServer"), "Protocol")) {
            json_t* legacy = json_object_get(json_object_get(root, "WebSocketServer"), "protocol");
            if (legacy) {
                log_this("Config", "Warning: Using legacy lowercase 'protocol' key, please update to 'Protocol'",
                    LOG_LEVEL_ALERT);
                success = success && PROCESS_STRING(root, &config->websocket, protocol, "WebSocketServer.protocol", "WebSocketServer");
            }
        }
    }

    // Validate the configuration if loaded successfully
    if (success) {
        success = (config_websocket_validate(&config->websocket) == 0);
    }

    return success;
}

// Initialize WebSocket configuration with default values
int config_websocket_init(WebSocketConfig* config) {
    if (!config) {
        log_this("Config", "WebSocket config pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    // Set default values
    config->enabled = true;
    config->enable_ipv6 = false;
    config->port = 5001;
    config->max_message_size = 1024 * 1024;  // 1MB
    config->exit_wait_seconds = 5;

    // Initialize string fields
    config->protocol = strdup("hydrogen");
    if (!config->protocol) {
        log_this("Config", "Failed to allocate WebSocket protocol", LOG_LEVEL_ERROR);
        return -1;
    }

    config->key = strdup("${env.WEBSOCKET_KEY}");
    if (!config->key) {
        log_this("Config", "Failed to allocate WebSocket key", LOG_LEVEL_ERROR);
        free(config->protocol);
        config->protocol = NULL;
        return -1;
    }

    return 0;
}

// Free resources allocated for WebSocket configuration
void config_websocket_cleanup(WebSocketConfig* config) {
    if (!config) {
        return;
    }

    // Free allocated strings
    if (config->protocol) {
        free(config->protocol);
        config->protocol = NULL;
    }

    if (config->key) {
        free(config->key);
        config->key = NULL;
    }

    // Zero out the structure
    memset(config, 0, sizeof(WebSocketConfig));
}

// Validate protocol string
static int validate_protocol(const char* protocol) {
    if (!protocol || !protocol[0]) {
        return -1;
    }

    // Protocol must be a valid identifier:
    // - Start with letter
    // - Contain only letters, numbers, and hyphens
    if (!((protocol[0] >= 'a' && protocol[0] <= 'z') ||
          (protocol[0] >= 'A' && protocol[0] <= 'Z'))) {
        return -1;
    }

    for (size_t i = 0; protocol[i]; i++) {
        char c = protocol[i];
        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '-')) {
            return -1;
        }
    }

    return 0;
}

// Validate key string
static int validate_key(const char* key) {
    if (!key || !key[0]) {
        return -1;
    }

    // Key must be:
    // - At least 8 characters long
    // - Contain only printable ASCII characters
    // - No spaces or control characters
    size_t len = strlen(key);
    if (len < 8) {
        return -1;
    }

    for (size_t i = 0; i < len; i++) {
        char c = key[i];
        if (c < 33 || c > 126) {  // Non-printable ASCII or space
            return -1;
        }
    }

    return 0;
}

// Validate WebSocket configuration values
int config_websocket_validate(const WebSocketConfig* config) {
    if (!config) {
        log_this("Config", "WebSocket config pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    // If WebSocket server is enabled, validate all settings
    if (config->enabled) {
        // Validate port number
        if (config->port < MIN_PORT || config->port > MAX_PORT) {
            log_this("Config", "Invalid WebSocket port (must be between 1024-65535)", LOG_LEVEL_ERROR);
            return -1;
        }

        // Validate protocol and key
        if (validate_protocol(config->protocol) != 0) {
            log_this("Config", "Invalid WebSocket protocol", LOG_LEVEL_ERROR);
            return -1;
        }

        if (validate_key(config->key) != 0) {
            log_this("Config", "Invalid WebSocket key", LOG_LEVEL_ERROR);
            return -1;
        }

        // Validate message size limits
        if (config->max_message_size < WEBSOCKET_MIN_MESSAGE_SIZE ||
            config->max_message_size > WEBSOCKET_MAX_MESSAGE_SIZE) {
            log_this("Config", "Invalid WebSocket message size limits", LOG_LEVEL_ERROR);
            return -1;
        }

        // Validate exit wait timeout
        if (config->exit_wait_seconds < MIN_EXIT_WAIT_SECONDS ||
            config->exit_wait_seconds > MAX_EXIT_WAIT_SECONDS) {
            log_this("Config", "Invalid WebSocket exit wait timeout", LOG_LEVEL_ERROR);
            return -1;
        }
    }

    return 0;
}