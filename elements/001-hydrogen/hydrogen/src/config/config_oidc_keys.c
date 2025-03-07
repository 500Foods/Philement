/*
 * OpenID Connect Keys Configuration Implementation
 */

#define _GNU_SOURCE  // For strdup

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>
#include "config_oidc_keys.h"
#include "config_string.h"

// Maximum allowed rotation interval (1 year)
#define MAX_ROTATION_INTERVAL_DAYS 365

int config_oidc_keys_init(OIDCKeysConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize with default values
    config->rotation_interval_days = DEFAULT_KEY_ROTATION_DAYS;
    config->encryption_enabled = DEFAULT_KEY_ENCRYPTION_ENABLED;
    
    // Allocate and set storage path
    config->storage_path = strdup(DEFAULT_KEY_STORAGE_PATH);
    if (!config->storage_path) {
        config_oidc_keys_cleanup(config);
        return -1;
    }

    return 0;
}

void config_oidc_keys_cleanup(OIDCKeysConfig* config) {
    if (!config) {
        return;
    }

    // Free allocated memory
    free(config->storage_path);

    // Zero out the structure
    memset(config, 0, sizeof(OIDCKeysConfig));
}

static int validate_storage_path(const char* path) {
    struct stat st;
    
    // Path must not be empty
    if (!path || !path[0]) {
        return -1;
    }

    // Path must be absolute
    if (path[0] != '/') {
        return -1;
    }

    // If path exists, check permissions
    if (stat(path, &st) == 0) {
        // Must be a directory
        if (!S_ISDIR(st.st_mode)) {
            return -1;
        }
        
        // Check read/write permissions
        if (access(path, R_OK | W_OK) != 0) {
            return -1;
        }
    }
    // If path doesn't exist, check if parent directory is writable
    else {
        char* parent_path = strdup(path);
        if (!parent_path) {
            return -1;
        }
        
        char* last_slash = strrchr(parent_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            
            // Check if parent directory exists and is writable
            if (stat(parent_path, &st) != 0 || !S_ISDIR(st.st_mode) ||
                access(parent_path, R_OK | W_OK) != 0) {
                free(parent_path);
                return -1;
            }
        }
        free(parent_path);
    }

    return 0;
}

int config_oidc_keys_validate(const OIDCKeysConfig* config) {
    if (!config) {
        return -1;
    }

    // Validate rotation interval
    if (config->rotation_interval_days <= 0 || 
        config->rotation_interval_days > MAX_ROTATION_INTERVAL_DAYS) {
        return -1;
    }

    // Validate storage path
    if (validate_storage_path(config->storage_path) != 0) {
        return -1;
    }

    return 0;
}