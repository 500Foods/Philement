/*
 * WebSocket Server Startup Module
 * 
 * Handles complex WebSocket server initialization including:
 * - Context creation and configuration
 * - Protocol and vhost setup
 * - Port fallback logic
 * - IPv6 configuration
 * - Logging setup
 */

 // Global includes 
#include <src/hydrogen.h>

// Local includes
#include "websocket_server.h"
#include "websocket_server_internal.h"
#include <netinet/in.h>
#include <arpa/inet.h> // Added for inet_pton

/* External variables */
extern AppConfig* app_config;
extern WebSocketServerContext *ws_context;

/* External functions */
extern int callback_http(struct lws *wsi, enum lws_callback_reasons reason,
                        void *user, void *in, size_t len);
extern int callback_hydrogen(struct lws *wsi, enum lws_callback_reasons reason,
                           void *user, void *in, size_t len);
extern void custom_lws_log(int level, const char *line);

// Validate WebSocket server initialization parameters
int validate_websocket_params(int port, const char* protocol, const char* key) {
    if (port <= 0 || port > 65535) {
        log_this(SR_WEBSOCKET, "Invalid port number: %d", LOG_LEVEL_ERROR, 1, port);
        return -1;
    }

    if (!protocol || strlen(protocol) == 0) {
        log_this(SR_WEBSOCKET, "Invalid protocol: NULL or empty", LOG_LEVEL_ERROR, 0);
        return -1;
    }

    if (!key || strlen(key) == 0) {
        log_this(SR_WEBSOCKET, "Invalid authentication key: NULL or empty", LOG_LEVEL_ERROR, 0);
        return -1;
    }

    return 0;
}

// Set up WebSocket protocol array
void setup_websocket_protocols(struct lws_protocols protocols[3], const char* protocol) {
    // HTTP protocol for upgrade requests
    protocols[0] = (struct lws_protocols){
        .name = "http",
        .callback = callback_http,
        .per_session_data_size = 0,
        .rx_buffer_size = 0,
        .id = 0,
        .user = NULL,
        .tx_packet_size = 0
    };

    // Custom protocol
    protocols[1] = (struct lws_protocols){
        .name = protocol,
        .callback = callback_hydrogen,
        .per_session_data_size = sizeof(WebSocketSessionData),
        .rx_buffer_size = 0,
        .id = 1,
        .user = NULL,
        .tx_packet_size = 0
    };

    // Terminator
    protocols[2] = (struct lws_protocols){ NULL, NULL, 0, 0, 0, NULL, 0 };
}

// Configure libwebsockets context creation info
void configure_lws_context_info(struct lws_context_creation_info* info,
                               struct lws_protocols* protocols,
                               WebSocketServerContext* context) {
    memset(info, 0, sizeof(*info));

    info->port = context->port;
    info->protocols = protocols;
    info->gid = (gid_t)-1;
    info->uid = (uid_t)-1;
    info->user = context;
    info->options = LWS_SERVER_OPTION_ALLOW_LISTEN_SHARE;
}

// Configure libwebsockets vhost creation info
void configure_lws_vhost_info(struct lws_context_creation_info* vhost_info,
                             int port, struct lws_protocols* protocols,
                             WebSocketServerContext* context) {
    memset(vhost_info, 0, sizeof(*vhost_info));

    vhost_info->port = port;
    vhost_info->protocols = protocols;
    vhost_info->vhost_name = "hydrogen";
    vhost_info->user = context;
    vhost_info->options = LWS_SERVER_OPTION_ALLOW_LISTEN_SHARE |
                         LWS_SERVER_OPTION_VALIDATE_UTF8 |
                         LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE |
                         LWS_SERVER_OPTION_SKIP_SERVER_CANONICAL_NAME;

    // Bind to all interfaces
    vhost_info->iface = NULL;
}

// Verify WebSocket port binding by testing socket creation and binding
int verify_websocket_port_binding(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        log_this(SR_WEBSOCKET, "Failed to create test socket: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        // Port is available, so vhost creation failed
        close(sock);
        log_this(SR_WEBSOCKET, "Port %d is available but vhost creation failed", LOG_LEVEL_ERROR, 1, port);
        return -1;
    } else {
        // Port is in use, likely by our server
        close(sock);
        log_this(SR_WEBSOCKET, "Port %d appears to be in use, assuming successful binding", LOG_LEVEL_STATE, 1, port);
        return 0;
    }
}

// Initialize the WebSocket server
int init_websocket_server(int port, const char* protocol, const char* key)
{
    // Validate parameters
    if (validate_websocket_params(port, protocol, key) != 0) {
        return -1;
    }

    // Create and initialize server context
    ws_context = ws_context_create(port, protocol, key);
    if (!ws_context) {
        log_this(SR_WEBSOCKET, "Failed to create server context", LOG_LEVEL_DEBUG, 0);
        return -1;
    }

    // Set up protocol array
    struct lws_protocols protocols[3];
    setup_websocket_protocols(protocols, protocol);

    // Configure libwebsockets context
    struct lws_context_creation_info info;
    configure_lws_context_info(&info, protocols, ws_context);

    // Set minimal logging
    int lws_level = LLL_ERR | LLL_WARN; // Errors and warnings only
    lws_set_log_level(lws_level, NULL);

    // Create libwebsockets context
    ws_context->lws_context = lws_create_context(&info);
    if (!ws_context->lws_context) {
        log_this(SR_WEBSOCKET, "Failed to create LWS context", LOG_LEVEL_ERROR, 0);
        ws_context_destroy(ws_context);
        ws_context = NULL;
        return -1;
    }

    // Configure and create vhost
    struct lws_context_creation_info vhost_info;
    configure_lws_vhost_info(&vhost_info, port, protocols, ws_context);

    log_this(SR_WEBSOCKET, "Binding to all interfaces (0.0.0.0:%d)", LOG_LEVEL_STATE, 1, port);

    ws_context->vhost_creating = 1;
    const struct lws_vhost *vhost = lws_create_vhost(ws_context->lws_context, &vhost_info);
    ws_context->vhost_creating = 0;

    if (!vhost) {
        log_this(SR_WEBSOCKET, "Failed to create vhost for 0.0.0.0:%d", LOG_LEVEL_ERROR, 1, port);
        lws_context_destroy(ws_context->lws_context);
        ws_context_destroy(ws_context);
        ws_context = NULL;
        return -1;
    }

    // Verify port binding
    if (verify_websocket_port_binding(port) != 0) {
        lws_context_destroy(ws_context->lws_context);
        ws_context_destroy(ws_context);
        ws_context = NULL;
        return -1;
    }

    ws_context->port = port;
    log_this(SR_WEBSOCKET, "Successfully bound to 0.0.0.0:%d", LOG_LEVEL_STATE, 1, port);
    log_this(SR_WEBSOCKET, "Server initialized on port %d with protocol %s", LOG_LEVEL_STATE, 2, ws_context->port, protocol);
    return 0;
}