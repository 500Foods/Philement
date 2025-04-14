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
    bool success = true;
    WebServerConfig* webserver = &config->webserver;

    // Zero out the structure
    memset(webserver, 0, sizeof(WebServerConfig));

    // Set defaults directly in the structure
    webserver->enable_ipv4 = true;
    webserver->enable_ipv6 = false;
    webserver->port = 8080;
    webserver->max_upload_size = 100 * 1024 * 1024;  // 100MB
    
    // Allocate and set default paths
    webserver->web_root = strdup("/var/www/html");
    webserver->upload_path = strdup("/upload");
    webserver->upload_dir = strdup("/var/uploads");

    success = PROCESS_SECTION(root, "WebServer");
    
    // Process network settings
    success = success && PROCESS_BOOL(root, webserver, enable_ipv4, "WebServer.EnableIPv4", "WebServer");
    success = success && PROCESS_BOOL(root, webserver, enable_ipv6, "WebServer.EnableIPv6", "WebServer");
    success = success && PROCESS_INT(root, webserver, port, "WebServer.Port", "WebServer");

    // Process paths
    success = success && PROCESS_STRING(root, webserver, web_root, "WebServer.WebRoot", "WebServer");
    success = success && PROCESS_STRING(root, webserver, upload_path, "WebServer.UploadPath", "WebServer");
    success = success && PROCESS_STRING(root, webserver, upload_dir, "WebServer.UploadDir", "WebServer");
    success = success && PROCESS_SIZE(root, webserver, max_upload_size, "WebServer.MaxUploadSize", "WebServer");

    return success;
}

void dump_webserver_config(const WebServerConfig* config) {
    if (!config) {
        DUMP_TEXT("", "Cannot dump NULL web server config");
        return;
    }

    // Network settings
    DUMP_BOOL("―― IPv4 Enabled", config->enable_ipv4);
    DUMP_BOOL("―― IPv6 Enabled", config->enable_ipv6);
    DUMP_INT("―― Port", config->port);
 
    // Paths
    DUMP_STRING("―― Web Root", config->web_root);
    DUMP_STRING("―― Upload Path", config->upload_path);
    DUMP_STRING("―― Upload Directory", config->upload_dir);
    DUMP_SIZE("―― Max Upload Size", config->max_upload_size);

}

void cleanup_webserver_config(WebServerConfig* config) {
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