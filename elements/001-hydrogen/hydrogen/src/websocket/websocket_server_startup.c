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
#include "../hydrogen.h"

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

// Initialize the WebSocket server
// int init_websocket_server(int port, const char* protocol, const char* key)
// {
//     // Create and initialize server context
//     ws_context = ws_context_create(port, protocol, key);
//     if (!ws_context) {
//         log_this(SR_WEBSOCKET, "Failed to create server context", LOG_LEVEL_DEBUG, 0);
//         return -1;
//     }

//     // Configure libwebsockets
//     struct lws_context_creation_info info;
//     memset(&info, 0, sizeof(info));

//     // Define protocol handlers - need HTTP support for upgrade requests
//     struct lws_protocols protocols[] = {
//         {
//             .name = "http",
//             .callback = callback_http,
//             .per_session_data_size = 0,
//             .rx_buffer_size = 0,
//             .id = 0,
//             .user = NULL,
//             .tx_packet_size = 0
//         },
//         {
//             .name = protocol,
//             .callback = callback_hydrogen,
//             .per_session_data_size = sizeof(WebSocketSessionData),
//             .rx_buffer_size = 0,
//             .id = 1,
//             .user = NULL,
//             .tx_packet_size = 0
//         },
//         { NULL, NULL, 0, 0, 0, NULL, 0 } /* terminator */
//     };

//     info.port = port;
//     info.protocols = protocols;
//     info.gid = (gid_t)-1;
//     info.uid = (uid_t)-1;
//     info.user = ws_context;  // Set context as user data
//     info.options = LWS_SERVER_OPTION_EXPLICIT_VHOSTS |
//                   LWS_SERVER_OPTION_ALLOW_LISTEN_SHARE |   // Enable SO_REUSEADDR for immediate rebinding
//                   LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;

//     // Configure IPv6 if enabled
//     if (app_config && app_config->websocket.enable_ipv6) {
//         info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
//         log_this(SR_WEBSOCKET, "IPv6 support enabled", LOG_LEVEL_STATE, 0);
//     }

//     log_this(SR_WEBSOCKET, "Configuring SO_REUSEADDR for immediate socket rebinding via LWS_SERVER_OPTION_ALLOW_LISTEN_SHARE", LOG_LEVEL_STATE, 0);

//     // Set context user data
//     lws_set_log_level(0, NULL);  // Reset logging before context creation

//     // Map numeric levels to libwebsockets levels
//     int lws_level = 0;
//     int config_level = LOG_LEVEL_ERROR;  // Default to ERROR level (minimal logging)
    
//     // Look up WebSockets-Lib subsystem level if configured
//     for (size_t i = 0; i < app_config->logging.console.subsystem_count; i++) {
//         if (strcmp(app_config->logging.console.subsystems[i].name, SR_WEBSOCKET_LIB) == 0) {
//             config_level = app_config->logging.console.subsystems[i].level;
//             break;
//         }
//     }
    
//     switch (config_level) {
//         case 0:  // TRACE - Maximum verbosity for deep debugging
//             lws_level = LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG | LLL_PARSER | LLL_HEADER;
//             break;
//         case 1:  // DEBUG - Verbose debugging information
//             lws_level = LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO | LLL_DEBUG;
//             break;
//         case 2:  // STATE - General information during normal operation
//             lws_level = LLL_ERR | LLL_WARN | LLL_NOTICE | LLL_INFO;
//             break;
//         case 3:  // ALERT - Important warnings
//             lws_level = LLL_ERR | LLL_WARN | LLL_NOTICE;
//             break;
//         case 4:  // ERROR - Errors only (default for minimal logging)
//             lws_level = LLL_ERR;
//             break;
//         case 5:  // FATAL - Critical errors only
//             lws_level = LLL_ERR;
//             break;
//         case 6:  // QUIET - No libwebsockets logging
//             lws_level = 0;
//             break;
//         default:
//             lws_level = LLL_ERR;  // Default to ERROR level only
//             break;
//     }

//     lws_set_log_level(lws_level, custom_lws_log);

//     // Create libwebsockets context
//     ws_context->lws_context = lws_create_context(&info);
//     if (!ws_context->lws_context) {
//         log_this(SR_WEBSOCKET, "Failed to create LWS context", LOG_LEVEL_DEBUG, 0);
//         ws_context_destroy(ws_context);
//         ws_context = NULL;
//         return -1;
//     }

//     // Create vhost with port fallback
//     struct lws_vhost *vhost = NULL;
//     int try_port = port;
//     int max_attempts = 10;

//     // Add initialization options
//     uint64_t vhost_options = LWS_SERVER_OPTION_VALIDATE_UTF8 |
//                             LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE |
//                             LWS_SERVER_OPTION_SKIP_SERVER_CANONICAL_NAME;  // Skip hostname checks during init

//     // Set vhost creation flag
//     ws_context->vhost_creating = 1;

//     for (int attempt = 0; attempt < max_attempts; attempt++) {
//         // Prepare vhost info
//         struct lws_context_creation_info vhost_info;
//         memset(&vhost_info, 0, sizeof(vhost_info));
//         vhost_info.port = try_port;
//         vhost_info.protocols = protocols;
//         vhost_info.options = vhost_options | LWS_SERVER_OPTION_ALLOW_LISTEN_SHARE;  // Enable SO_REUSEADDR
//         // Configure interface binding based on Network.Available configuration
//         bool has_enabled_interfaces = false;

//         // Check if we have any enabled network interfaces
//         if (app_config && app_config->network.available_interfaces &&
//             app_config->network.available_interfaces_count > 0) {

//             // Get network interface information to find enabled interfaces
//             network_info_t *enabled_interfaces = filter_enabled_interfaces(get_network_info(), app_config);
//             if (enabled_interfaces && enabled_interfaces->count > 0) {
//                 has_enabled_interfaces = true;

//                 // Set interface binding - use first enabled interface's IP
//                 if (app_config->websocket.enable_ipv6 && enabled_interfaces->interfaces[0].ip_count > 0) {
//                     // Find first IPv6 address
//                     for (int i = 0; i < enabled_interfaces->interfaces[0].ip_count; i++) {
//                         if (strchr(enabled_interfaces->interfaces[0].ips[i], ':') != NULL) {
//                             vhost_info.iface = enabled_interfaces->interfaces[0].ips[i];
//                             log_this(SR_WEBSOCKET, "Binding to IPv6 interface %s (%s)", LOG_LEVEL_STATE, 2, enabled_interfaces->interfaces[0].name, vhost_info.iface);
//                             break;
//                         }
//                     }
//                     if (!vhost_info.iface) vhost_info.iface = "::"; // Fallback if no IPv6 found
//                 } else if (enabled_interfaces->interfaces[0].ip_count > 0) {
//                     // Find first IPv4 address
//                     for (int i = 0; i < enabled_interfaces->interfaces[0].ip_count; i++) {
//                         if (strchr(enabled_interfaces->interfaces[0].ips[i], ':') == NULL) {
//                             vhost_info.iface = enabled_interfaces->interfaces[0].ips[i];
//                             log_this(SR_WEBSOCKET, "Binding to IPv4 interface %s (%s)", LOG_LEVEL_STATE, 2, enabled_interfaces->interfaces[0].name, vhost_info.iface);
//                             break;
//                         }
//                     }
//                     if (!vhost_info.iface) vhost_info.iface = "0.0.0.0"; // Fallback if no IPv4 found
//                 }
//             }
//             if (enabled_interfaces) free_network_info(enabled_interfaces);
//         }

//         // Fallback to all interfaces if no enabled interfaces found
//         if (!has_enabled_interfaces || !vhost_info.iface) {
//             vhost_info.iface = app_config && app_config->websocket.enable_ipv6 ? "::" : "0.0.0.0";
//             if (!has_enabled_interfaces) {
//                 log_this(SR_WEBSOCKET, "No enabled interfaces found, binding to all interfaces (%s,4,3,2,1,0)", LOG_LEVEL_ALERT, 1, vhost_info.iface);
//             } else {
//                 log_this(SR_WEBSOCKET, "Using fallback interface binding (%s)", LOG_LEVEL_STATE, 1, vhost_info.iface);
//             }
//         }
//         vhost_info.vhost_name = "hydrogen";  // Set explicit vhost name
//         vhost_info.user = ws_context;        // Pass context to callbacks

//         // Try to create vhost
//         vhost = lws_create_vhost(ws_context->lws_context, &vhost_info);
//         if (vhost) {
//             ws_context->port = try_port;
//             log_this(SR_WEBSOCKET, "Successfully bound to port %d", LOG_LEVEL_STATE, 1, try_port);
//             break;
//         }
        
//         // Check port availability
//         int sock = socket(AF_INET, SOCK_STREAM, 0);
//         if (sock != -1) {
//             struct sockaddr_in addr;
//             addr.sin_family = AF_INET;
//             addr.sin_port = htons((uint16_t)try_port);
//             addr.sin_addr.s_addr = INADDR_ANY;
            
//             if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
//                 // Port is available but vhost creation failed for other reasons
//                 close(sock);
//                 log_this(SR_WEBSOCKET, "Port %d is available but vhost creation failed", LOG_LEVEL_ALERT, 1, try_port);
//             } else {
//                 close(sock);
//                 log_this(SR_WEBSOCKET, "Port %d is in use, trying next port", 1, LOG_LEVEL_STATE, try_port);
//             }
//         }
        
//         try_port++;
//     }

//     // Clear vhost creation flag
//     ws_context->vhost_creating = 0;

//     // Handle vhost creation result
//     if (!vhost) {
//         log_this(SR_WEBSOCKET, "Failed to create vhost after multiple attempts", LOG_LEVEL_DEBUG, 0);
//         ws_context_destroy(ws_context);
//         ws_context = NULL;
//         return -1;
//     }

//     log_this(SR_WEBSOCKET, "Vhost creation completed successfully", LOG_LEVEL_STATE, 0);

//     // Log initialization with protocol validation
//     if (protocol) {
//         log_this(SR_WEBSOCKET, "Server initialized on port %d with protocol %s", LOG_LEVEL_STATE, 2, ws_context->port, protocol);
//     } else {
//         log_this(SR_WEBSOCKET, "Server initialized on port %d", LOG_LEVEL_STATE, 1, ws_context->port);
//     }
//     return 0;
// }

// Initialize the WebSocket server
int init_websocket_server(int port, const char* protocol, const char* key)
{
    // Create and initialize server context
    ws_context = ws_context_create(port, protocol, key);
    if (!ws_context) {
        log_this(SR_WEBSOCKET, "Failed to create server context", LOG_LEVEL_DEBUG, 0);
        return -1;
    }

    // Configure libwebsockets
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

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
        { NULL, NULL, 0, 0, 0, NULL, 0 }
    };

    info.port = port;
    info.protocols = protocols;
    info.gid = (gid_t)-1;
    info.uid = (uid_t)-1;
    info.user = ws_context;
    info.options = LWS_SERVER_OPTION_ALLOW_LISTEN_SHARE;

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

    // Create vhost
    struct lws_context_creation_info vhost_info;
    memset(&vhost_info, 0, sizeof(vhost_info));
    vhost_info.port = port;
    vhost_info.protocols = protocols;
    vhost_info.vhost_name = "hydrogen";
    vhost_info.user = ws_context;
    vhost_info.options = LWS_SERVER_OPTION_ALLOW_LISTEN_SHARE |
                        LWS_SERVER_OPTION_VALIDATE_UTF8 |
                        LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE |
                        LWS_SERVER_OPTION_SKIP_SERVER_CANONICAL_NAME;

    // Bind to all interfaces (0.0.0.0)
    vhost_info.iface = NULL;
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

    // Verify binding by testing the port
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock != -1) {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons((uint16_t)port);
        addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0

        if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            // Port is available, so vhost creation failed
            close(sock);
            log_this(SR_WEBSOCKET, "Port %d is available but vhost creation failed", LOG_LEVEL_ERROR, 1, port);
            lws_context_destroy(ws_context->lws_context);
            ws_context_destroy(ws_context);
            ws_context = NULL;
            return -1;
        } else {
            // Port is in use, likely by our server
            close(sock);
            log_this(SR_WEBSOCKET, "Port %d appears to be in use, assuming successful binding", LOG_LEVEL_STATE, 1, port);
        }
    } else {
        log_this(SR_WEBSOCKET, "Failed to create test socket: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
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