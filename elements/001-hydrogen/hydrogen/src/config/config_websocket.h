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

// WebSocket connection timeouts structure
struct WebSocketConnectionTimeouts {
    int shutdown_wait_seconds;       // How long to wait for shutdown
    int service_loop_delay_ms;       // Service loop delay in milliseconds
    int connection_cleanup_ms;       // Connection cleanup interval
    int exit_wait_seconds;           // How long to wait for connections to close on exit
};
typedef struct WebSocketConnectionTimeouts WebSocketConnectionTimeouts;

// WebSocket configuration structure
struct WebSocketConfig {
    bool enable_ipv4;               // Whether WebSocket server is enabled
    bool enable_ipv6;               // Whether to enable IPv6 support
    int lib_log_level;              // Library log level
    int port;                       // Port to listen on
    char* key;                      // WebSocket key for authentication
    char* protocol;                 // WebSocket protocol identifier
    size_t max_message_size;        // Maximum allowed message size
    WebSocketConnectionTimeouts connection_timeouts; // Connection timeout settings
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
 * Dump WebSocket configuration
 *
 * This function outputs the current WebSocket configuration values
 * in a formatted way, using consistent indentation and logging categories.
 *
 * @param config Pointer to WebSocketConfig structure to dump
 */
void dump_websocket_config(const WebSocketConfig* config);

/*
 * Clean up WebSocket configuration
 *
 * This function frees all resources allocated during configuration loading.
 * It safely handles NULL pointers and partial initialization.
 * After cleanup, the structure is zeroed to prevent use-after-free.
 *
 * @param config Pointer to WebSocketConfig structure to cleanup
 */
void cleanup_websocket_config(WebSocketConfig* config);

#endif /* HYDROGEN_CONFIG_WEBSOCKET_H */
