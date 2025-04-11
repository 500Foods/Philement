/*
 * WebSocket Configuration
 *
 * Defines the configuration structure and handlers for the WebSocket subsystem.
 * Includes settings for WebSocket server operation, security, and message handling.
 */

#ifndef HYDROGEN_CONFIG_WEBSOCKET_H
#define HYDROGEN_CONFIG_WEBSOCKET_H

#include <stddef.h>
#include <stdbool.h>
#include <jansson.h>
#include "config_forward.h"  // For AppConfig forward declaration

// WebSocket configuration structure
struct WebSocketConfig {
    bool enabled;             // Whether WebSocket server is enabled
    bool enable_ipv6;        // Whether to enable IPv6 support
    int port;                // Port to listen on
    char* key;               // WebSocket key for authentication
    char* protocol;          // WebSocket protocol identifier
    size_t max_message_size; // Maximum allowed message size
    int exit_wait_seconds;   // How long to wait for connections to close on exit
};
typedef struct WebSocketConfig WebSocketConfig;

/*
 * Load WebSocket configuration from JSON
 *
 * This function loads the WebSocket configuration from the provided JSON root,
 * applying any environment variable overrides and using secure defaults
 * where values are not specified.
 *
 * @param root JSON root object containing configuration
 * @param config Pointer to AppConfig structure to update
 * @return true if successful, false on error
 */
bool load_websocket_config(json_t* root, AppConfig* config);

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
 * - Validates port number (must be between 1024-65535)
 * - Checks protocol and key strings
 * - Verifies message size limits (1KB minimum)
 * - Validates exit wait timeout (1-60 seconds)
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