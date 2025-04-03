/*
 * Web Server Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <linux/limits.h>
#include "config_webserver.h"
#include "../types/config_string.h"

// Validation limits
#define MIN_PORT 1024
#define MAX_PORT 65535
#define MIN_THREAD_POOL_SIZE 1
#define MAX_THREAD_POOL_SIZE 64
#define MIN_CONNECTIONS 1
#define MAX_CONNECTIONS 10000
#define MIN_CONNECTIONS_PER_IP 1
#define MAX_CONNECTIONS_PER_IP 1000
#define MIN_CONNECTION_TIMEOUT 1
#define MAX_CONNECTION_TIMEOUT 3600

int config_webserver_init(WebServerConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize basic settings
    config->enable_ipv4 = DEFAULT_WEB_ENABLE_IPV4;
    config->enable_ipv6 = DEFAULT_WEB_ENABLE_IPV6;
    config->port = DEFAULT_WEB_PORT;
    
    // Initialize string fields
    config->web_root = strdup(DEFAULT_WEB_ROOT);
    config->upload_path = strdup(DEFAULT_UPLOAD_PATH);
    config->upload_dir = strdup(DEFAULT_UPLOAD_DIR);
    
    if (!config->web_root || !config->upload_path || !config->upload_dir) {
        config_webserver_cleanup(config);
        return -1;
    }

    // Initialize upload settings
    config->max_upload_size = DEFAULT_MAX_UPLOAD_SIZE;

    // Initialize thread and connection settings
    config->thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
    config->max_connections = DEFAULT_MAX_CONNECTIONS;
    config->max_connections_per_ip = DEFAULT_MAX_CONNECTIONS_PER_IP;
    config->connection_timeout = DEFAULT_CONNECTION_TIMEOUT;

    return 0;
}

void config_webserver_cleanup(WebServerConfig* config) {
    if (!config) {
        return;
    }

    // Free basic string fields
    free(config->web_root);
    free(config->upload_path);
    free(config->upload_dir);

    // Zero out the structure
    memset(config, 0, sizeof(WebServerConfig));
}

static int validate_directory(const char* path, int write_access) {
    struct stat st;
    
    if (!path || !path[0]) {
        return -1;
    }

    // Path must be absolute
    if (path[0] != '/') {
        return -1;
    }

    // If path exists, check if it's a directory with proper permissions
    if (stat(path, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            return -1;
        }
        
        // Check read access
        if (access(path, R_OK) != 0) {
            return -1;
        }
        
        // Check write access if required
        if (write_access && access(path, W_OK) != 0) {
            return -1;
        }
    }
    // If path doesn't exist, check if parent directory is writable
    else if (write_access) {
        char parent_path[PATH_MAX];
        strncpy(parent_path, path, sizeof(parent_path) - 1);
        parent_path[sizeof(parent_path) - 1] = '\0';
        
        char* last_slash = strrchr(parent_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            
            if (stat(parent_path, &st) != 0 || !S_ISDIR(st.st_mode) ||
                access(parent_path, W_OK) != 0) {
                return -1;
            }
        }
    }

    return 0;
}


int config_webserver_validate(const WebServerConfig* config) {
    if (!config) {
        return -1;
    }

    // If either IPv4 or IPv6 is enabled, validate all settings
    if (config->enable_ipv4 || config->enable_ipv6) {
        // Validate port number
        if (config->port < MIN_PORT || config->port > MAX_PORT) {
            return -1;
        }

        // Validate thread and connection settings
        if (config->thread_pool_size < MIN_THREAD_POOL_SIZE ||
            config->thread_pool_size > MAX_THREAD_POOL_SIZE ||
            config->max_connections < MIN_CONNECTIONS ||
            config->max_connections > MAX_CONNECTIONS ||
            config->max_connections_per_ip < MIN_CONNECTIONS_PER_IP ||
            config->max_connections_per_ip > MAX_CONNECTIONS_PER_IP ||
            config->connection_timeout < MIN_CONNECTION_TIMEOUT ||
            config->connection_timeout > MAX_CONNECTION_TIMEOUT) {
            return -1;
        }

        // Validate paths
        if (validate_directory(config->web_root, 0) != 0 ||  // Read-only
            validate_directory(config->upload_dir, 1) != 0) { // Read-write
            return -1;
        }


        // Validate upload path
        if (!config->upload_path || !config->upload_path[0] ||
            config->upload_path[0] != '/') {
            return -1;
        }

        // Validate max upload size (must be > 0)
        if (config->max_upload_size == 0) {
            return -1;
        }

        // Validate connection limits relationship
        if (config->max_connections_per_ip > config->max_connections) {
            return -1;
        }
    }

    return 0;
}