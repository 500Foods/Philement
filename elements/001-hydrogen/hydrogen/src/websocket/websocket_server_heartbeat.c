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
extern AppConfig *app_config;

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
        log_this(SR_WEBSOCKET, "[WS] PING already pending for %s, skipping",
                 LOG_LEVEL_TRACE, 1, session->request_ip);
        return;
    }

    // Send the ping frame
    int result = ws_write_ping_frame(wsi);

    if (result < 0) {
        log_this(SR_WEBSOCKET, "[WS] PING failed to send to %s",
                 LOG_LEVEL_TRACE, 1, session->request_ip);
        return;
    }

    session->last_ping_sent = time(NULL);
    session->ping_pending = true;
    session->heartbeat_ping_due = false;

    log_this(SR_WEBSOCKET, "[WS] PING sent to %s",
             LOG_LEVEL_STATE, 1, session->request_ip);
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
    double rtt = 0;
    if (session->last_ping_sent > 0) {
        rtt = difftime(now, session->last_ping_sent);
    }

    session->last_pong_received = now;
    session->ping_pending = false;

    log_this(SR_WEBSOCKET, "[WS] PONG received from %s (RTT: %.3fs)",
             LOG_LEVEL_STATE, 2,
             session->request_ip,
             rtt);
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
            log_this(SR_WEBSOCKET, "[WS] UNHEALTHY connection to %s: pong timeout (%.1fs > %ds)",
                     LOG_LEVEL_TRACE, 2, session->request_ip, time_since_ping, pong_timeout_seconds);
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

/*
 * Arm the per-connection LWS timer used to schedule heartbeats.
 */
void ws_arm_heartbeat_timer(struct lws *wsi)
{
    if (!wsi || !app_config || !app_config->websocket.heartbeat.enabled) {
        return;
    }

    int interval = app_config->websocket.heartbeat.ping_interval_seconds;
    if (interval < 1) {
        interval = 1;
    }

    lws_set_timer_usecs(wsi, (lws_usec_t)interval * 1000000LL);
}

/*
 * Handle LWS_CALLBACK_TIMER: health-check, schedule a ping, re-arm timer.
 * Returns -1 to close the connection when unhealthy.
 */
int ws_handle_heartbeat_timer(struct lws *wsi, WebSocketSessionData *session)
{
    if (!wsi || !session || !app_config || !app_config->websocket.heartbeat.enabled) {
        return 0;
    }

    int pong_timeout = app_config->websocket.heartbeat.pong_timeout_seconds;
    if (pong_timeout < 1) {
        pong_timeout = 1;
    }

    if (!ws_check_connection_health(wsi, session, pong_timeout)) {
        log_this(SR_WEBSOCKET, "[WS] Closing unhealthy connection to %s (pong timeout)",
                 LOG_LEVEL_STATE, 1, session->request_ip);
        return -1;
    }

    int stale = app_config->websocket.heartbeat.stale_connection_seconds;
    if (stale > 0 && session->last_pong_received > 0) {
        double idle = difftime(time(NULL), session->last_pong_received);
        if (idle > (double)stale) {
            log_this(SR_WEBSOCKET, "[WS] Closing stale connection to %s (%.0fs > %ds)",
                     LOG_LEVEL_STATE, 2, session->request_ip, idle, stale);
            return -1;
        }
    }

    session->heartbeat_ping_due = true;
    ws_request_heartbeat_ping(wsi, session);
    ws_arm_heartbeat_timer(wsi);
    return 0;
}

/*
 * Send a due heartbeat ping from the writable callback.
 */
void ws_maybe_send_heartbeat_ping(struct lws *wsi, WebSocketSessionData *session)
{
    if (!wsi || !session || !session->heartbeat_ping_due) {
        return;
    }

    ws_send_ping(wsi, session);
}
