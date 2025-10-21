/*
 * Terminal WebSocket Validation
 *
 * This module handles validation and simple getter functions for terminal
 * WebSocket connections, including request validation, authentication checks,
 * and protocol information.
 */

#include <src/hydrogen.h>
#include <src/globals.h>
#include <src/logging/logging.h>
#include <src/config/config_terminal.h>
#include "terminal_websocket.h"
#include "terminal_session.h"

#include <string.h>

// WebSocket protocol name for terminal connections
#define TERMINAL_WS_PROTOCOL "terminal"

/**
 * Check if WebSocket upgrade request is for terminal
 *
 * This function validates whether an HTTP request is attempting to upgrade
 * to a WebSocket connection for terminal access.
 *
 * @param connection MHD connection object
 * @param method HTTP method (should be "GET")
 * @param url Request URL
 * @param config Terminal configuration
 * @return true if request is valid terminal WebSocket upgrade, false otherwise
 */
bool is_terminal_websocket_request(struct MHD_Connection *connection __attribute__((unused)),
                                  const char *method,
                                  const char *url,
                                  const TerminalConfig *config) {
    // Must be GET request
    if (!method || strcmp(method, "GET") != 0) {
        return false;
    }

    // Check if URL is terminal WebSocket endpoint
    if (!url || !config || !config->web_path) {
        return false;
    }

    // Check if URL matches expected pattern: /terminal/ws
    char expected_path[256];
    if (snprintf(expected_path, sizeof(expected_path), "%s/ws", config->web_path) >= (int)sizeof(expected_path)) {
        return false;
    }

    if (strcmp(url, expected_path) != 0) {
        return false;
    }

    // Check for required WebSocket headers
    const char *upgrade = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Upgrade");
    const char *connection_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Connection");
    const char *sec_websocket_key = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Sec-WebSocket-Key");

    if (!upgrade || !connection_header || !sec_websocket_key) {
        return false;
    }

    // Validate WebSocket upgrade headers
    if (strcasecmp(upgrade, "websocket") != 0) {
        return false;
    }

    // Connection header should include "Upgrade"
    if (strcasestr(connection_header, "upgrade") == NULL) {
        return false;
    }

    log_this(SR_TERMINAL, "Valid WebSocket upgrade request detected for URL: %s", LOG_LEVEL_STATE, 1, url);
    return true;
}

/**
 * Get WebSocket subprotocol for terminal connections
 *
 * @return Protocol string for WebSocket handshake
 */
const char *get_terminal_websocket_protocol(void) {
    return TERMINAL_WS_PROTOCOL;
}

/**
 * Check if session manager requires WebSocket authentication
 *
 * @param config Terminal configuration
 * @return Always false for now (no authentication required)
 */
bool terminal_websocket_requires_auth(const TerminalConfig *config __attribute__((unused))) {
    return false; // For now, no authentication required
}

/**
 * Get current WebSocket connection statistics
 *
 * @param connections Pointer to store active connection count
 * @param max_connections Pointer to store maximum connection limit
 * @return true on success, false on failure
 */
bool get_websocket_connection_stats(size_t *connections, size_t *max_connections) {
    return get_session_manager_stats(connections, max_connections);
}