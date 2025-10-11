/*
 * WebSocket Authentication Handler
 *
 * Implements connection authentication using a key-based scheme:
 * - Validates authentication headers
 * - Manages session authentication state
 * - Provides security logging
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "websocket_server_internal.h"

// External reference to the server context
extern WebSocketServerContext *ws_context;

int ws_handle_authentication(struct lws *wsi, WebSocketSessionData *session, const char *auth_header)
{
    if (!session || !auth_header || !ws_context) {
        log_this(SR_WEBSOCKET, "Invalid authentication parameters", LOG_LEVEL_ERROR, 0);
        return -1;
    }

    // Already authenticated
    if (session->authenticated) {
        return 0;
    }

    // Check if the authorization scheme is correct
    if (strncmp(auth_header, HYDROGEN_AUTH_SCHEME " ", strlen(HYDROGEN_AUTH_SCHEME) + 1) != 0) {
        log_this(SR_WEBSOCKET, "Invalid authentication scheme", LOG_LEVEL_ALERT, 0);
        return -1;
    }

    // Extract and validate the key
    const char *key = auth_header + strlen(HYDROGEN_AUTH_SCHEME) + 1;
    
    // Update client info before validation for better logging
    ws_update_client_info(wsi, session);
    
    if (strcmp(key, ws_context->auth_key) != 0) {
        log_this(SR_WEBSOCKET, "Authentication failed for client %s (%s)", LOG_LEVEL_ALERT, 2, session->request_ip, session->request_app);
        return -1;
    }

    // Authentication successful
    session->authenticated = true;
    log_this(SR_WEBSOCKET, "Client authenticated successfully: %s (%s)", LOG_LEVEL_STATE, 2 ,session->request_ip, session->request_app);

    return 0;
}

// Helper function to check if a session is authenticated
bool ws_is_authenticated(const WebSocketSessionData *session)
{
    return session && session->authenticated;
}

// Helper function to clear authentication state
void ws_clear_authentication(WebSocketSessionData *session)
{
    if (session) {
        session->authenticated = false;
        if (session->authenticated_key) {
            free(session->authenticated_key);
            session->authenticated_key = NULL;
        }
    }
}
