/*
 * Notify Configuration Implementation
 */

// Standard C headers
#include <stdlib.h>
#include <string.h>

// Project headers
#include "config_notify.h"

int config_notify_init(NotifyConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize with default values
    config->enabled = false;  // Disabled by default for safety
    config->notifier = NULL;  // Must be set by configuration
    
    // Initialize SMTP config with defaults
    config->smtp.host = NULL;
    config->smtp.port = DEFAULT_SMTP_PORT;
    config->smtp.username = NULL;
    config->smtp.password = NULL;
    config->smtp.use_tls = DEFAULT_SMTP_TLS;

    return 0;
}

void config_notify_cleanup(NotifyConfig* config) {
    if (!config) {
        return;
    }

    // Free allocated strings
    free(config->notifier);
    free(config->smtp.host);
    free(config->smtp.username);
    free(config->smtp.password);

    // Zero out the structure
    memset(config, 0, sizeof(NotifyConfig));
}

int config_notify_validate(const NotifyConfig* config) {
    if (!config) {
        return -1;
    }

    // If enabled, validate required fields
    if (config->enabled) {
        // Notifier type must be specified
        if (!config->notifier || !config->notifier[0]) {
            return -1;
        }

        // If SMTP notifier, validate SMTP config
        if (strcmp(config->notifier, "SMTP") == 0) {
            // Host is required
            if (!config->smtp.host || !config->smtp.host[0]) {
                return -1;
            }

            // Port must be valid
            if (config->smtp.port <= 0 || config->smtp.port > 65535) {
                return -1;
            }

            // Username and password must both be present or both absent
            if ((!config->smtp.username && config->smtp.password) ||
                (config->smtp.username && !config->smtp.password)) {
                return -1;
            }
        } else {
            // Unknown notifier type
            return -1;
        }
    }

    return 0;
}