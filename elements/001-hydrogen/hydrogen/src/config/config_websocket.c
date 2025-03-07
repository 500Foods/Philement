/*
 * WebSocket Configuration Implementation
 */

#define _GNU_SOURCE  // For strdup

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "config_websocket.h"
#include "config_string.h"

// Validation limits
#define MIN_PORT 1024
#define MAX_PORT 65535
#define MIN_EXIT_WAIT_SECONDS 1
#define MAX_EXIT_WAIT_SECONDS 60
#define MIN_MESSAGE_SIZE 1024        // 1KB
#define MAX_MESSAGE_SIZE 0x40000000  // 1GB

int config_websocket_init(WebSocketConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize basic settings
    config->enabled = DEFAULT_WEBSOCKET_ENABLED;
    config->enable_ipv6 = DEFAULT_WEBSOCKET_ENABLE_IPV6;
    config->port = DEFAULT_WEBSOCKET_PORT;
    config->max_message_size = DEFAULT_MAX_MESSAGE_SIZE;
    config->exit_wait_seconds = DEFAULT_EXIT_WAIT_SECONDS;

    // Initialize string fields
    config->protocol = strdup(DEFAULT_WEBSOCKET_PROTOCOL);
    config->key = strdup(DEFAULT_WEBSOCKET_KEY);

    if (!config->protocol || !config->key) {
        config_websocket_cleanup(config);
        return -1;
    }

    return 0;
}

void config_websocket_cleanup(WebSocketConfig* config) {
    if (!config) {
        return;
    }

    // Free allocated strings
    free(config->protocol);
    free(config->key);

    // Zero out the structure
    memset(config, 0, sizeof(WebSocketConfig));
}

static int validate_protocol(const char* protocol) {
    if (!protocol || !protocol[0]) {
        return -1;
    }

    // Protocol must be a valid identifier:
    // - Start with letter
    // - Contain only letters, numbers, and hyphens
    // - Be at least 1 character long
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

int config_websocket_validate(const WebSocketConfig* config) {
    if (!config) {
        return -1;
    }

    // If WebSocket server is enabled, validate all settings
    if (config->enabled) {
        // Validate port number
        if (config->port < MIN_PORT || config->port > MAX_PORT) {
            return -1;
        }

        // Validate protocol and key
        if (validate_protocol(config->protocol) != 0 ||
            validate_key(config->key) != 0) {
            return -1;
        }

        // Validate message size limits
        if (config->max_message_size < MIN_MESSAGE_SIZE ||
            config->max_message_size > MAX_MESSAGE_SIZE) {
            return -1;
        }

        // Validate exit wait timeout
        if (config->exit_wait_seconds < MIN_EXIT_WAIT_SECONDS ||
            config->exit_wait_seconds > MAX_EXIT_WAIT_SECONDS) {
            return -1;
        }
    }

    return 0;
}