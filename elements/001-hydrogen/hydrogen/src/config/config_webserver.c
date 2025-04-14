/*
 * Web Server Configuration Implementation
 *
 * Implements the configuration handlers for the web server subsystem.
 */

#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "config_webserver.h"
#include "config_utils.h"
#include "../logging/logging.h"

bool load_webserver_config(json_t* root, AppConfig* config) {
    if (!root || !config) {
        return false;
    }

    WebServerConfig* webserver = &config->webserver;

    // Zero out the structure
    memset(webserver, 0, sizeof(WebServerConfig));

    // Set defaults directly in the structure
    webserver->enable_ipv4 = true;
    webserver->enable_ipv6 = false;
    webserver->port = 8080;
    webserver->thread_pool_size = 4;
    webserver->max_connections = 1000;
    webserver->max_connections_per_ip = 100;
    webserver->connection_timeout = 60;
    webserver->max_upload_size = 100 * 1024 * 1024;  // 100MB
    
    // Allocate and set default paths
    webserver->web_root = strdup("/var/www/html");
    webserver->upload_path = strdup("/upload");
    webserver->upload_dir = strdup("/var/uploads");

    
    // Process network settings
    PROCESS_BOOL(root, webserver, enable_ipv4, "WebServer.enable_ipv4", "WebServer");
    PROCESS_BOOL(root, webserver, enable_ipv6, "WebServer.enable_ipv6", "WebServer");
    PROCESS_INT(root, webserver, port, "WebServer.port", "WebServer");

    // Process paths
    PROCESS_STRING(root, webserver, web_root, "WebServer.web_root", "WebServer");
    PROCESS_STRING(root, webserver, upload_path, "WebServer.upload_path", "WebServer");
    PROCESS_STRING(root, webserver, upload_dir, "WebServer.upload_dir", "WebServer");
    PROCESS_SIZE(root, webserver, max_upload_size, "WebServer.max_upload_size", "WebServer");

    // Process thread and connection settings
    PROCESS_INT(root, webserver, thread_pool_size, "WebServer.thread_pool_size", "WebServer");
    PROCESS_INT(root, webserver, max_connections, "WebServer.max_connections", "WebServer");
    PROCESS_INT(root, webserver, max_connections_per_ip, "WebServer.max_connections_per_ip", "WebServer");
    PROCESS_INT(root, webserver, connection_timeout, "WebServer.connection_timeout", "WebServer");

    return true;
}

void dump_webserver_config(const WebServerConfig* config) {
    if (!config) {
        DUMP_TEXT("", "Cannot dump NULL web server config");
        return;
    }

    // Network settings
    DUMP_TEXT("――", "Network Settings");
    DUMP_BOOL("IPv4 Enabled", config->enable_ipv4);
    DUMP_BOOL("IPv6 Enabled", config->enable_ipv6);
    DUMP_INT("Port", config->port);

    // Paths
    DUMP_TEXT("――", "Paths");
    DUMP_STRING("Web Root", config->web_root);
    DUMP_STRING("Upload Path", config->upload_path);
    DUMP_STRING("Upload Directory", config->upload_dir);
    DUMP_SIZE("Max Upload Size", config->max_upload_size);

    // Thread and connection settings
    DUMP_TEXT("――", "Thread and Connection Settings");
    DUMP_INT("Thread Pool Size", config->thread_pool_size);
    DUMP_INT("Max Connections", config->max_connections);
    DUMP_INT("Max Connections per IP", config->max_connections_per_ip);
    DUMP_INT("Connection Timeout", config->connection_timeout);
}

void config_webserver_cleanup(WebServerConfig* config) {
    if (!config) {
        return;
    }

    free(config->web_root);
    free(config->upload_path);
    free(config->upload_dir);

    // Note: swagger and api configs are owned by AppConfig
    // and cleaned up separately

    memset(config, 0, sizeof(WebServerConfig));
}