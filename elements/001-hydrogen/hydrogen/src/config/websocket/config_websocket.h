/*
 * WebSocket Configuration
 *
 * Defines the configuration structure and defaults for the WebSocket subsystem.
 * This includes settings for WebSocket server operation and message handling.
 */

#ifndef HYDROGEN_CONFIG_WEBSOCKET_H
#define HYDROGEN_CONFIG_WEBSOCKET_H

#include <stddef.h>

// Project headers
#include "../config_forward.h"

// Default values for WebSocket server
#define DEFAULT_WEBSOCKET_ENABLED 1
#define DEFAULT_WEBSOCKET_ENABLE_IPV6 0
#define DEFAULT_WEBSOCKET_PORT 5001
#define DEFAULT_WEBSOCKET_PROTOCOL "hydrogen"
#define DEFAULT_WEBSOCKET_KEY "hydrogen-websocket"
#define DEFAULT_MAX_MESSAGE_SIZE (1024 * 1024)  // 1MB
#define DEFAULT_EXIT_WAIT_SECONDS 5

// WebSocket configuration structure
struct WebSocketConfig {
    int enabled;              // Whether WebSocket server is enabled
    int enable_ipv6;         // Whether to enable IPv6 support
    int port;                // Port to listen on
    char* key;               // WebSocket key for authentication
    char* protocol;          // WebSocket protocol identifier
    size_t max_message_size; // Maximum allowed message size
    int exit_wait_seconds;   // How long to wait for connections to close on exit
};

/*
 * Initialize WebSocket configuration with default values
 *
 * This function initializes a new WebSocketConfig structure with default
 * values that provide a secure and performant baseline configuration.
 *
 * @param config Pointer to WebSocketConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 * - If memory allocation fails for any string field
 */
int config_websocket_init(WebSocketConfig* config);

/*
 * Free resources allocated for WebSocket configuration
 *
 * This function frees all resources allocated by config_websocket_init.
 * It safely handles NULL pointers and partial initialization.
 * After cleanup, the structure is zeroed to prevent use-after-free.
 *
 * @param config Pointer to WebSocketConfig structure to cleanup
 */
void config_websocket_cleanup(WebSocketConfig* config);

/*
 * Validate WebSocket configuration values
 *
 * This function performs comprehensive validation of the configuration:
 * - Validates port number
 * - Checks protocol and key strings
 * - Verifies message size limits
 * - Validates exit wait timeout
 *
 * @param config Pointer to WebSocketConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If enabled but required fields are missing
 * - If any numeric value is out of valid range
 * - If protocol or key strings are invalid
 */
int config_websocket_validate(const WebSocketConfig* config);

#endif /* HYDROGEN_CONFIG_WEBSOCKET_H */