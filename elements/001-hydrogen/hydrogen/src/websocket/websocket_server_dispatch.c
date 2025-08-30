/*
 * WebSocket Callback Dispatcher
 *
 * Routes WebSocket events to appropriate handlers:
 * - Connection lifecycle events
 * - Authentication events
 * - Message processing
 * - Server state management
 */

// Global includes 
#include "../hydrogen.h"

// Local includes
#include "websocket_server_internal.h"

// External reference to the server context
extern WebSocketServerContext *ws_context;

int ws_callback_dispatch(struct lws *wsi, enum lws_callback_reasons reason,
                        void *user, void *in, size_t len)
{
    WebSocketSessionData *session = (WebSocketSessionData *)user;

    // Protocol lifecycle callbacks - handle these first and independently
    if (reason == LWS_CALLBACK_PROTOCOL_INIT ||
        reason == LWS_CALLBACK_PROTOCOL_DESTROY) {
        // These are critical for clean shutdown
        if (reason == LWS_CALLBACK_PROTOCOL_DESTROY) {
            if (ws_context) {
                // Final protocol cleanup
                pthread_mutex_lock(&ws_context->mutex);
                
                // Log any remaining connections
                if (ws_context->active_connections > 0) {
                    log_this(SR_WEBSOCKET, "Protocol destroy with %d active connections",
                            LOG_LEVEL_ALERT, true, true, true, ws_context->active_connections);
                }
                
                // Force clear connections and notify all waiting threads
                ws_context->active_connections = 0;
                pthread_cond_broadcast(&ws_context->cond);
                
                pthread_mutex_unlock(&ws_context->mutex);
                
                log_this(SR_WEBSOCKET, "Protocol cleanup complete", LOG_LEVEL_STATE);
            } else {
                // Context already cleaned up, which is also valid
                log_this(SR_WEBSOCKET, "Protocol destroy with no context", LOG_LEVEL_STATE);
            }
        }
        return 0;
    }

    // Get server context if available
    WebSocketServerContext *ctx = ws_context;
    if (!ctx) {
        ctx = (WebSocketServerContext *)lws_context_user(lws_get_context(wsi));
    }

    // During vhost creation or early initialization, allow all callbacks
    if (!ctx || (ctx && ctx->vhost_creating)) {
        return 0;  // Accept all callbacks during initialization
    }

    // Normal operation requires valid context
    if (!ws_context) {
        log_this(SR_WEBSOCKET, "No server context available for callback %d", LOG_LEVEL_ERROR, reason);
        return -1;
    }

    // Handle shutdown state first
    if (ws_context->shutdown) {
        switch (reason) {
            // Allow cleanup callbacks during shutdown
            case LWS_CALLBACK_WSI_DESTROY:
            case LWS_CALLBACK_CLOSED:
            case LWS_CALLBACK_PROTOCOL_DESTROY:
                if (reason == LWS_CALLBACK_CLOSED || reason == LWS_CALLBACK_WSI_DESTROY) {
                    // During shutdown, some sessions might be already cleaned up
                    if (!session) {
                        log_this(SR_WEBSOCKET, "Connection cleanup with no session during shutdown", 
                                LOG_LEVEL_STATE, true, true, true);
                        return 0;
                    }
                    int result = ws_handle_connection_closed(wsi, session);
                    // Broadcast completion if this was the last connection
                    if (result == 0) {
                        pthread_mutex_lock(&ws_context->mutex);
                        if (ws_context->active_connections == 0) {
                            log_this(SR_WEBSOCKET, "Last connection closed, notifying waiters", 
                                    LOG_LEVEL_STATE, true, true, true);
                            pthread_cond_broadcast(&ws_context->cond);
                        }
                        pthread_mutex_unlock(&ws_context->mutex);
                    }
                    return result;
                }
                return 0;

            // Allow system callbacks during shutdown without session validation
            case LWS_CALLBACK_GET_THREAD_ID:
            case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
            case LWS_CALLBACK_ADD_POLL_FD:
            case LWS_CALLBACK_DEL_POLL_FD:
            case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
            case LWS_CALLBACK_LOCK_POLL:
            case LWS_CALLBACK_UNLOCK_POLL:
                return 0;

            // Reject new connections during shutdown
            case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
            case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
            case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED:
            case LWS_CALLBACK_ESTABLISHED:
                return -1;  // Silently reject during shutdown

            // All other callback types - ignore during shutdown
            case LWS_CALLBACK_PROTOCOL_INIT:
            case LWS_CALLBACK_WSI_CREATE:
            case LWS_CALLBACK_WSI_TX_CREDIT_GET:
            case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS:
            case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS:
            case LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION:
            case LWS_CALLBACK_SSL_INFO:
            case LWS_CALLBACK_OPENSSL_PERFORM_SERVER_CERT_VERIFICATION:
            case LWS_CALLBACK_HTTP:
            case LWS_CALLBACK_HTTP_BODY:
            case LWS_CALLBACK_HTTP_BODY_COMPLETION:
            case LWS_CALLBACK_HTTP_FILE_COMPLETION:
            case LWS_CALLBACK_HTTP_WRITEABLE:
            case LWS_CALLBACK_CLOSED_HTTP:
            case LWS_CALLBACK_FILTER_HTTP_CONNECTION:
            case LWS_CALLBACK_ADD_HEADERS:
            case LWS_CALLBACK_VERIFY_BASIC_AUTHORIZATION:
            case LWS_CALLBACK_CHECK_ACCESS_RIGHTS:
            case LWS_CALLBACK_PROCESS_HTML:
            case LWS_CALLBACK_HTTP_BIND_PROTOCOL:
            case LWS_CALLBACK_HTTP_DROP_PROTOCOL:
            case LWS_CALLBACK_HTTP_CONFIRM_UPGRADE:
            case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
            case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
            case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ:
            case LWS_CALLBACK_RECEIVE_CLIENT_HTTP:
            case LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
            case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
            case LWS_CALLBACK_CLIENT_HTTP_REDIRECT:
            case LWS_CALLBACK_CLIENT_HTTP_BIND_PROTOCOL:
            case LWS_CALLBACK_CLIENT_HTTP_DROP_PROTOCOL:
            case LWS_CALLBACK_SERVER_WRITEABLE:
            case LWS_CALLBACK_RECEIVE:
            case LWS_CALLBACK_RECEIVE_PONG:
            case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
            case LWS_CALLBACK_CONFIRM_EXTENSION_OKAY:
            case LWS_CALLBACK_WS_SERVER_BIND_PROTOCOL:
            case LWS_CALLBACK_WS_SERVER_DROP_PROTOCOL:
            case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            case LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH:
            case LWS_CALLBACK_WS_CLIENT_BIND_PROTOCOL:
            case LWS_CALLBACK_WS_CLIENT_DROP_PROTOCOL:
            case LWS_CALLBACK_CLIENT_RECEIVE:
            case LWS_CALLBACK_CLIENT_RECEIVE_PONG:
            case LWS_CALLBACK_CLIENT_WRITEABLE:
            case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
            case LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED:
            case LWS_CALLBACK_WS_EXT_DEFAULTS:
            case LWS_CALLBACK_SESSION_INFO:
            case LWS_CALLBACK_GS_EVENT:
            case LWS_CALLBACK_HTTP_PMO:
            case LWS_CALLBACK_RAW_PROXY_CLI_RX:
            case LWS_CALLBACK_RAW_PROXY_SRV_RX:
            case LWS_CALLBACK_RAW_PROXY_CLI_CLOSE:
            case LWS_CALLBACK_RAW_PROXY_SRV_CLOSE:
            case LWS_CALLBACK_RAW_PROXY_CLI_WRITEABLE:
            case LWS_CALLBACK_RAW_PROXY_SRV_WRITEABLE:
            case LWS_CALLBACK_RAW_PROXY_CLI_ADOPT:
            case LWS_CALLBACK_RAW_PROXY_SRV_ADOPT:
            case LWS_CALLBACK_RAW_PROXY_CLI_BIND_PROTOCOL:
            case LWS_CALLBACK_RAW_PROXY_SRV_BIND_PROTOCOL:
            case LWS_CALLBACK_RAW_PROXY_CLI_DROP_PROTOCOL:
            case LWS_CALLBACK_RAW_PROXY_SRV_DROP_PROTOCOL:
            case LWS_CALLBACK_RAW_RX:
            case LWS_CALLBACK_RAW_CLOSE:
            case LWS_CALLBACK_RAW_WRITEABLE:
            case LWS_CALLBACK_RAW_ADOPT:
            case LWS_CALLBACK_RAW_CONNECTED:
            case LWS_CALLBACK_RAW_SKT_BIND_PROTOCOL:
            case LWS_CALLBACK_RAW_SKT_DROP_PROTOCOL:
            case LWS_CALLBACK_RAW_ADOPT_FILE:
            case LWS_CALLBACK_RAW_RX_FILE:
            case LWS_CALLBACK_RAW_WRITEABLE_FILE:
            case LWS_CALLBACK_RAW_CLOSE_FILE:
            case LWS_CALLBACK_RAW_FILE_BIND_PROTOCOL:
            case LWS_CALLBACK_RAW_FILE_DROP_PROTOCOL:
            case LWS_CALLBACK_TIMER:
            case LWS_CALLBACK_CHILD_CLOSING:
            case LWS_CALLBACK_CONNECTING:
            case LWS_CALLBACK_VHOST_CERT_AGING:
            case LWS_CALLBACK_VHOST_CERT_UPDATE:
            case LWS_CALLBACK_MQTT_NEW_CLIENT_INSTANTIATED:
            case LWS_CALLBACK_MQTT_IDLE:
            case LWS_CALLBACK_MQTT_CLIENT_ESTABLISHED:
            case LWS_CALLBACK_MQTT_SUBSCRIBED:
            case LWS_CALLBACK_MQTT_CLIENT_WRITEABLE:
            case LWS_CALLBACK_MQTT_CLIENT_RX:
            case LWS_CALLBACK_MQTT_UNSUBSCRIBED:
            case LWS_CALLBACK_MQTT_DROP_PROTOCOL:
            case LWS_CALLBACK_MQTT_CLIENT_CLOSED:
            case LWS_CALLBACK_MQTT_ACK:
            case LWS_CALLBACK_MQTT_RESEND:
            case LWS_CALLBACK_MQTT_UNSUBSCRIBE_TIMEOUT:
            case LWS_CALLBACK_MQTT_SHADOW_TIMEOUT:
            case LWS_CALLBACK_USER:
            case LWS_CALLBACK_CLIENT_ESTABLISHED:
            case LWS_CALLBACK_CLIENT_CLOSED:
            case LWS_CALLBACK_CGI:
            case LWS_CALLBACK_CGI_TERMINATED:
            case LWS_CALLBACK_CGI_STDIN_DATA:
            case LWS_CALLBACK_CGI_STDIN_COMPLETED:
            case LWS_CALLBACK_CGI_PROCESS_ATTACH:
                // During shutdown, log but don't error on missing session
                if (!session) {
                    log_this(SR_WEBSOCKET, "Ignoring callback %d during shutdown (no session)", 
                            LOG_LEVEL_STATE, true, true, true, reason);
                }
                return -1;  // Silently reject other callbacks during shutdown
        }
    }

    // Session validation for normal operation
    // Allow these callbacks to proceed without session data during connection establishment
    if (!session && 
        reason != LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED &&
        reason != LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION &&
        reason != LWS_CALLBACK_FILTER_NETWORK_CONNECTION &&
        reason != LWS_CALLBACK_HTTP_CONFIRM_UPGRADE &&
        reason != LWS_CALLBACK_FILTER_HTTP_CONNECTION &&
        reason != LWS_CALLBACK_WS_SERVER_BIND_PROTOCOL &&
        reason != LWS_CALLBACK_WS_SERVER_DROP_PROTOCOL) {
        log_this(SR_WEBSOCKET, "Invalid session data for callback %d", LOG_LEVEL_DEBUG, reason);
        return -1;
    }

    // Normal operation dispatch
    switch (reason) {
        // Connection Lifecycle
        case LWS_CALLBACK_ESTABLISHED:
            return ws_handle_connection_established(wsi, session);

        case LWS_CALLBACK_CLOSED:
        case LWS_CALLBACK_WSI_DESTROY:
        case LWS_CALLBACK_CLOSED_HTTP:
            return ws_handle_connection_closed(wsi, session);

        // Authentication and Security
        case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
            if (ws_context->vhost_creating) {
                return 0;  // Allow during vhost creation
            }
            {
                char buf[256];
                int length = lws_hdr_total_length(wsi, WSI_TOKEN_HTTP_AUTHORIZATION);
                if (length <= 0) {
                    log_this(SR_WEBSOCKET, "Missing authorization header", LOG_LEVEL_ALERT);
                    return -1;
                }

                lws_hdr_copy(wsi, buf, sizeof(buf), WSI_TOKEN_HTTP_AUTHORIZATION);
                
                // During protocol filtering, session isn't initialized yet
                // Validate authentication directly without session
                if (strncmp(buf, "Key ", 4) != 0) {
                    log_this(SR_WEBSOCKET, "Invalid authentication scheme", LOG_LEVEL_ALERT);
                    return -1;
                }
                
                const char *key = buf + 4;  // Skip "Key "
                if (strcmp(key, ws_context->auth_key) != 0) {
                    log_this(SR_WEBSOCKET, "Authentication failed during protocol filtering", LOG_LEVEL_ALERT);
                    return -1;
                }
                
                log_this(SR_WEBSOCKET, "Authentication successful during protocol filtering", LOG_LEVEL_STATE);
                return 0;
            }

        // Message Processing
        case LWS_CALLBACK_RECEIVE:
            return ws_handle_receive(wsi, session, in, len);

        case LWS_CALLBACK_SERVER_WRITEABLE:
            return 0;  // Handle if we implement write queueing

        // Connection Setup
        case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
            return 0;  // Basic network-level filtering

        case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED:
            if (session) {
                session->authenticated = 0;
                session->connection_time = time(NULL);
            }
            return 0;

        // Protocol Management
        case LWS_CALLBACK_WS_SERVER_BIND_PROTOCOL:
        case LWS_CALLBACK_WS_SERVER_DROP_PROTOCOL:
            return 0;  // Protocol attach/detach events

        // HTTP Upgrade
        case LWS_CALLBACK_HTTP_CONFIRM_UPGRADE:
        case LWS_CALLBACK_FILTER_HTTP_CONNECTION:
            return 0;  // Allow upgrades during normal operation

        // All other callbacks - handled but ignored during normal operation
        case LWS_CALLBACK_PROTOCOL_INIT:
        case LWS_CALLBACK_PROTOCOL_DESTROY:
        case LWS_CALLBACK_WSI_CREATE:
        case LWS_CALLBACK_WSI_TX_CREDIT_GET:
        case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS:
        case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_SERVER_VERIFY_CERTS:
        case LWS_CALLBACK_OPENSSL_PERFORM_CLIENT_CERT_VERIFICATION:
        case LWS_CALLBACK_SSL_INFO:
        case LWS_CALLBACK_OPENSSL_PERFORM_SERVER_CERT_VERIFICATION:
        case LWS_CALLBACK_HTTP:
        case LWS_CALLBACK_HTTP_BODY:
        case LWS_CALLBACK_HTTP_BODY_COMPLETION:
        case LWS_CALLBACK_HTTP_FILE_COMPLETION:
        case LWS_CALLBACK_HTTP_WRITEABLE:
        case LWS_CALLBACK_ADD_HEADERS:
        case LWS_CALLBACK_VERIFY_BASIC_AUTHORIZATION:
        case LWS_CALLBACK_CHECK_ACCESS_RIGHTS:
        case LWS_CALLBACK_PROCESS_HTML:
        case LWS_CALLBACK_HTTP_BIND_PROTOCOL:
        case LWS_CALLBACK_HTTP_DROP_PROTOCOL:
        case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
        case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
        case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ:
        case LWS_CALLBACK_RECEIVE_CLIENT_HTTP:
        case LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
        case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
        case LWS_CALLBACK_CLIENT_HTTP_REDIRECT:
        case LWS_CALLBACK_CLIENT_HTTP_BIND_PROTOCOL:
        case LWS_CALLBACK_CLIENT_HTTP_DROP_PROTOCOL:
        case LWS_CALLBACK_RECEIVE_PONG:
        case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
        case LWS_CALLBACK_CONFIRM_EXTENSION_OKAY:
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        case LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH:
        case LWS_CALLBACK_WS_CLIENT_BIND_PROTOCOL:
        case LWS_CALLBACK_WS_CLIENT_DROP_PROTOCOL:
        case LWS_CALLBACK_CLIENT_RECEIVE:
        case LWS_CALLBACK_CLIENT_RECEIVE_PONG:
        case LWS_CALLBACK_CLIENT_WRITEABLE:
        case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
        case LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED:
        case LWS_CALLBACK_WS_EXT_DEFAULTS:
        case LWS_CALLBACK_SESSION_INFO:
        case LWS_CALLBACK_GS_EVENT:
        case LWS_CALLBACK_HTTP_PMO:
        case LWS_CALLBACK_GET_THREAD_ID:
        case LWS_CALLBACK_ADD_POLL_FD:
        case LWS_CALLBACK_DEL_POLL_FD:
        case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
        case LWS_CALLBACK_LOCK_POLL:
        case LWS_CALLBACK_UNLOCK_POLL:
        case LWS_CALLBACK_RAW_PROXY_CLI_RX:
        case LWS_CALLBACK_RAW_PROXY_SRV_RX:
        case LWS_CALLBACK_RAW_PROXY_CLI_CLOSE:
        case LWS_CALLBACK_RAW_PROXY_SRV_CLOSE:
        case LWS_CALLBACK_RAW_PROXY_CLI_WRITEABLE:
        case LWS_CALLBACK_RAW_PROXY_SRV_WRITEABLE:
        case LWS_CALLBACK_RAW_PROXY_CLI_ADOPT:
        case LWS_CALLBACK_RAW_PROXY_SRV_ADOPT:
        case LWS_CALLBACK_RAW_PROXY_CLI_BIND_PROTOCOL:
        case LWS_CALLBACK_RAW_PROXY_SRV_BIND_PROTOCOL:
        case LWS_CALLBACK_RAW_PROXY_CLI_DROP_PROTOCOL:
        case LWS_CALLBACK_RAW_PROXY_SRV_DROP_PROTOCOL:
        case LWS_CALLBACK_RAW_RX:
        case LWS_CALLBACK_RAW_CLOSE:
        case LWS_CALLBACK_RAW_WRITEABLE:
        case LWS_CALLBACK_RAW_ADOPT:
        case LWS_CALLBACK_RAW_CONNECTED:
        case LWS_CALLBACK_RAW_SKT_BIND_PROTOCOL:
        case LWS_CALLBACK_RAW_SKT_DROP_PROTOCOL:
        case LWS_CALLBACK_RAW_ADOPT_FILE:
        case LWS_CALLBACK_RAW_RX_FILE:
        case LWS_CALLBACK_RAW_WRITEABLE_FILE:
        case LWS_CALLBACK_RAW_CLOSE_FILE:
        case LWS_CALLBACK_RAW_FILE_BIND_PROTOCOL:
        case LWS_CALLBACK_RAW_FILE_DROP_PROTOCOL:
        case LWS_CALLBACK_TIMER:
        case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
        case LWS_CALLBACK_CHILD_CLOSING:
        case LWS_CALLBACK_CONNECTING:
        case LWS_CALLBACK_VHOST_CERT_AGING:
        case LWS_CALLBACK_VHOST_CERT_UPDATE:
        case LWS_CALLBACK_MQTT_NEW_CLIENT_INSTANTIATED:
        case LWS_CALLBACK_MQTT_IDLE:
        case LWS_CALLBACK_MQTT_CLIENT_ESTABLISHED:
        case LWS_CALLBACK_MQTT_SUBSCRIBED:
        case LWS_CALLBACK_MQTT_CLIENT_WRITEABLE:
        case LWS_CALLBACK_MQTT_CLIENT_RX:
        case LWS_CALLBACK_MQTT_UNSUBSCRIBED:
        case LWS_CALLBACK_MQTT_DROP_PROTOCOL:
        case LWS_CALLBACK_MQTT_CLIENT_CLOSED:
        case LWS_CALLBACK_MQTT_ACK:
        case LWS_CALLBACK_MQTT_RESEND:
        case LWS_CALLBACK_MQTT_UNSUBSCRIBE_TIMEOUT:
        case LWS_CALLBACK_MQTT_SHADOW_TIMEOUT:
        case LWS_CALLBACK_USER:
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
        case LWS_CALLBACK_CLIENT_CLOSED:
        case LWS_CALLBACK_CGI:
        case LWS_CALLBACK_CGI_TERMINATED:
        case LWS_CALLBACK_CGI_STDIN_DATA:
        case LWS_CALLBACK_CGI_STDIN_COMPLETED:
        case LWS_CALLBACK_CGI_PROCESS_ATTACH:
            // Log unhandled callback for debugging
            log_this(SR_WEBSOCKET, "Unhandled callback reason: %d", LOG_LEVEL_STATE, reason);
            return 0;  // Accept unhandled callbacks during normal operation
    }
    
    // Should never reach here, but ensure function returns a value
    return 0;
}
