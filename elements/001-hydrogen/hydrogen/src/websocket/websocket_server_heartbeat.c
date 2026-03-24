/*
 * WebSocket Heartbeat Implementation
 *
 * Implements ping/pong heartbeat mechanism for WebSocket connections.
 * This ensures connections remain alive through proxies and load balancers
 * (like Traefik in DOKS clusters) and detects dead connections.
 *
 * Note: This implementation works with the session data to track
 * connection health. The actual ping frames are sent via the writable
 * callback mechanism.
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "websocket_server_internal.h"
#include "websocket_server_message.h"

// External reference to the server context
extern WebSocketServerContext *ws_context;

// Logging source for heartbeat
static const char* SR_WEBSOCKET_HEARTBEAT = "WS_HEARTBEAT";

// Forward declaration
void ws_request_heartbeat_ping(struct lws *wsi, const WebSocketSessionData *session);

/*
 * Send a WebSocket ping frame to the client
 *
 * This function sends a ping frame and updates the session's heartbeat tracking.
 * The client should respond with a pong frame.
 *
 * @param wsi The WebSocket connection
 * @param session The session data for this connection
 */
void ws_send_ping(struct lws *wsi, WebSocketSessionData *session)
{
    if (!wsi || !session || !ws_context) {
        return;
    }

    // Don't send if we're already waiting for a pong
    if (session->ping_pending) {
        log_this(SR_WEBSOCKET_HEARTBEAT, "Ping already pending for %s, skipping",
                 LOG_LEVEL_DEBUG, 1, session->request_ip);
        return;
    }

    // Send the ping frame
    int result = ws_write_ping_frame(wsi);

    if (result < 0) {
        log_this(SR_WEBSOCKET_HEARTBEAT, "Failed to send ping to %s",
                 LOG_LEVEL_ERROR, 1, session->request_ip);
        return;
    }

    session->last_ping_sent = time(NULL);
    session->ping_pending = true;

    log_this(SR_WEBSOCKET_HEARTBEAT, "Ping sent to %s",
             LOG_LEVEL_DEBUG, 1, session->request_ip);
}

/*
 * Handle received pong frame from client
 *
 * Updates the session's heartbeat tracking when a pong is received.
 *
 * @param session The session data for this connection
 */
void ws_handle_pong_received(WebSocketSessionData *session)
{
    if (!session) {
        return;
    }

    time_t now = time(NULL);
    session->last_pong_received = now;
    session->ping_pending = false;

    // Calculate round trip time if we have a ping time
    if (session->last_ping_sent > 0) {
        double rtt = difftime(now, session->last_ping_sent);
        log_this(SR_WEBSOCKET_HEARTBEAT, "Pong received from %s (RTT: %.1fs)",
                 LOG_LEVEL_DEBUG, 2, session->request_ip, rtt);
    } else {
        log_this(SR_WEBSOCKET_HEARTBEAT, "Pong received from %s (unsolicited)",
                 LOG_LEVEL_DEBUG, 1, session->request_ip);
    }
}

/*
 * Check if a connection is still healthy
 *
 * Checks if the connection has exceeded the pong timeout or stale connection threshold.
 *
 * @param wsi The WebSocket connection
 * @param session The session data for this connection
 * @param pong_timeout_seconds How long to wait for pong before considering connection unhealthy
 * @return true if connection is healthy, false if it should be closed
 */
bool ws_check_connection_health(const struct lws *wsi, const WebSocketSessionData *session, int pong_timeout_seconds)
{
    if (!wsi || !session || !ws_context) {
        return false;
    }

    time_t now = time(NULL);

    // If we're waiting for a pong and have exceeded timeout, connection is dead
    if (session->ping_pending && session->last_ping_sent > 0) {
        double time_since_ping = difftime(now, session->last_ping_sent);
        if (time_since_ping > pong_timeout_seconds) {
            log_this(SR_WEBSOCKET_HEARTBEAT, "Connection to %s unhealthy: pong timeout (%.1fs > %ds)",
                     LOG_LEVEL_ALERT, 2, session->request_ip, time_since_ping, pong_timeout_seconds);
            return false;
        }
    }

    return true;
}

/*
 * Request a heartbeat ping for a connection
 *
 * This function marks the connection as needing a ping to be sent.
 * The actual ping will be sent when the connection becomes writable.
 *
 * @param wsi The WebSocket connection
 * @param session The session data for this connection
 */
void ws_request_heartbeat_ping(struct lws *wsi, const WebSocketSessionData *session)
{
    if (!wsi || !session) {
        return;
    }

    // Request writable callback to send the ping
    lws_callback_on_writable(wsi);
}
