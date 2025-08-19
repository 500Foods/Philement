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

 #include "../hydrogen.h"

/* System headers */
#include <sys/socket.h>
#include <netinet/in.h>

// Project headers
#include "websocket_server.h"
#include "websocket_server_internal.h"
#
/* External variables */
extern AppConfig* app_config;
extern WebSocketServerContext *ws_context;

/* External functions */
extern int callback_http(struct lws *wsi, enum lws_callback_reasons reason,
                        void *user, void *in, size_t len);
extern int callback_hydrogen(struct lws *wsi, enum lws_callback_reasons reason,
                           void *user, void *in, size_t len);
extern void custom_lws_log(int level, const char *line);

// Initialize the WebSocket server
int init_websocket_server(int port, const char* protocol, const char* key)
{
    // Create and initialize server context
    ws_context = ws_context_create(port, protocol, key);
    if (!ws_context) {
        log_this("WebSocket", "Failed to create server context", LOG_LEVEL_DEBUG);
        return -1;
    }

    // Configure libwebsockets
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    // Define protocol handlers - need HTTP support for upgrade requests
    struct lws_protocols protocols[] = {
        {
            .name = "http",
            .callback = callback_http,
            .per_session_data_size = 0,
            .rx_buffer_size = 0,
            .id = 0,
            .user = NULL,
            .tx_packet_size = 0
        },
        {
            .name = protocol,
            .callback = callback_hydrogen,
            .per_session_data_size = sizeof(WebSocketSessionData),
            .rx_buffer_size = 0,
            .id = 1,
            .user = NULL,
            .tx_packet_size = 0
        },
        { NULL, NULL, 0, 0, 0, NULL, 0 } /* terminator */
    };

    info.port = port;
    info.protocols = protocols;
    info.gid = (gid_t)-1;
    info.uid = (uid_t)-1;
    info.user = ws_context;  // Set context as user data
    info.options = LWS_SERVER_OPTION_EXPLICIT_VHOSTS |
                  LWS_SERVER_OPTION_ALLOW_LISTEN_SHARE |   // Enable SO_REUSEADDR for immediate rebinding
                  LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

    // Configure IPv6 if enabled
    if (app_config && app_config->websocket.enable_ipv6) {
        info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
        log_this("WebSocket", "IPv6 support enabled", LOG_LEVEL_STATE);
    }

    log_this("WebSocket", "Configuring SO_REUSEADDR for immediate socket rebinding via LWS_SERVER_OPTION_ALLOW_LISTEN_SHARE", LOG_LEVEL_STATE);

    // Set context user data
    lws_set_log_level(0, NULL);  // Reset logging before context creation

    // Map numeric levels to libwebsockets levels
    int lws_level = 0;
    int config_level = LOG_LEVEL_ERROR;  // Default to ERROR level (minimal logging)
    
    // Look up WebSockets-Lib subsystem level if configured
    for (size_t i = 0; i < app_config->logging.console.subsystem_count; i++) {
        if (strcmp(app_config->logging.console.subsystems[i].name, "WebSockets-Lib") == 0) {
            config_level = app_config->logging.console.subsystems[i].level;
            break;
        }
    }
    
    switch (config_level) {
        case 0:  // TRACE - Maximum verbosity for deep debugging
            lws_level = LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG | LLL_PARSER | LLL_HEADER;
            break;
        case 1:  // DEBUG - Verbose debugging information
            lws_level = LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG;
            break;
        case 2:  // STATE - General information during normal operation
            lws_level = LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO;
            break;
        case 3:  // ALERT - Important warnings
            lws_level = LLL_ERR | LLL_WARN | LLL_NOTICE;
            break;
        case 4:  // ERROR - Errors only (default for minimal logging)
            lws_level = LLL_ERR;
            break;
        case 5:  // FATAL - Critical errors only
            lws_level = LLL_ERR;
            break;
        case 6:  // QUIET - No libwebsockets logging
            lws_level = 0;
            break;
        default:
            lws_level = LLL_ERR;  // Default to ERROR level only
            break;
    }

    lws_set_log_level(lws_level, custom_lws_log);

    // Create libwebsockets context
    ws_context->lws_context = lws_create_context(&info);
    if (!ws_context->lws_context) {
        log_this("WebSocket", "Failed to create LWS context", LOG_LEVEL_DEBUG);
        ws_context_destroy(ws_context);
        ws_context = NULL;
        return -1;
    }

    // Create vhost with port fallback
    struct lws_vhost *vhost = NULL;
    int try_port = port;
    int max_attempts = 10;

    // Add initialization options
    uint64_t vhost_options = LWS_SERVER_OPTION_VALIDATE_UTF8 |
                            LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE |
                            LWS_SERVER_OPTION_SKIP_SERVER_CANONICAL_NAME;  // Skip hostname checks during init

    // Set vhost creation flag
    ws_context->vhost_creating = 1;

    for (int attempt = 0; attempt < max_attempts; attempt++) {
        // Prepare vhost info
        struct lws_context_creation_info vhost_info;
        memset(&vhost_info, 0, sizeof(vhost_info));
        vhost_info.port = try_port;
        vhost_info.protocols = protocols;
        vhost_info.options = vhost_options | LWS_SERVER_OPTION_ALLOW_LISTEN_SHARE;  // Enable SO_REUSEADDR
        vhost_info.iface = app_config->websocket.enable_ipv6 ? "::" : "0.0.0.0";
        vhost_info.vhost_name = "hydrogen";  // Set explicit vhost name
        vhost_info.user = ws_context;        // Pass context to callbacks

        // Try to create vhost
        vhost = lws_create_vhost(ws_context->lws_context, &vhost_info);
        if (vhost) {
            ws_context->port = try_port;
            log_this("WebSocket", "Successfully bound to port %d", LOG_LEVEL_STATE, try_port);
            break;
        }
        
        // Check port availability
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock != -1) {
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons((uint16_t)try_port);
            addr.sin_addr.s_addr = INADDR_ANY;
            
            if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                // Port is available but vhost creation failed for other reasons
                close(sock);
                log_this("WebSocket", "Port %d is available but vhost creation failed", LOG_LEVEL_ALERT, try_port);
            } else {
                close(sock);
                log_this("WebSocket", "Port %d is in use, trying next port", LOG_LEVEL_STATE, try_port);
            }
        }
        
        try_port++;
    }

    // Clear vhost creation flag
    ws_context->vhost_creating = 0;

    // Handle vhost creation result
    if (!vhost) {
        log_this("WebSocket", "Failed to create vhost after multiple attempts", LOG_LEVEL_DEBUG);
        ws_context_destroy(ws_context);
        ws_context = NULL;
        return -1;
    }

    log_this("WebSocket", "Vhost creation completed successfully", LOG_LEVEL_STATE);

    // Log initialization with protocol validation
    if (protocol) {
        log_this("WebSocket", "Server initialized on port %d with protocol %s", 
                 LOG_LEVEL_STATE, ws_context->port, protocol);
    } else {
        log_this("WebSocket", "Server initialized on port %d", 
                 LOG_LEVEL_STATE, ws_context->port);
    }
    return 0;
}
