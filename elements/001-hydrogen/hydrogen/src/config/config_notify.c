/*
 * Notify Configuration Implementation
 *
 * Implements the configuration handlers for the notification subsystem,
 * including JSON parsing, environment variable handling, and validation.
 */

#include <stdlib.h>
#include <string.h>
#include "config_notify.h"
#include "config.h"
#include "config_utils.h"
#include "../logging/logging.h"


// Load notification configuration from JSON
bool load_notify_config(json_t* root, AppConfig* config) {
    // Initialize with defaults
    config->notify = (NotifyConfig){
        .enabled = true,
        .notifier = "none",
        .smtp = {
            .host = NULL,
            .port = DEFAULT_SMTP_PORT,
            .username = NULL,
            .password = NULL,
            .use_tls = DEFAULT_SMTP_TLS,
            .timeout = DEFAULT_SMTP_TIMEOUT,
            .max_retries = DEFAULT_SMTP_MAX_RETRIES,
            .from_address = NULL
        }
    };

    // Process all config items in sequence
    bool success = true;

    // Process main notify section
    success = PROCESS_SECTION(root, "Notify");
    success = success && PROCESS_BOOL(root, &config->notify, enabled, "Notify.Enabled", "Notify");
    success = success && PROCESS_STRING(root, &config->notify, notifier, "Notify.Notifier", "Notify");

    // Process SMTP subsection if present
    if (success) {
        success = PROCESS_SECTION(root, "Notify.SMTP");
        success = success && PROCESS_STRING(root, &config->notify.smtp, host, "Notify.SMTP.Host", "Notify");
        success = success && PROCESS_INT(root, &config->notify.smtp, port, "Notify.SMTP.Port", "Notify");
        success = success && PROCESS_SENSITIVE(root, &config->notify.smtp, username, "Notify.SMTP.Username", "Notify");
        success = success && PROCESS_SENSITIVE(root, &config->notify.smtp, password, "Notify.SMTP.Password", "Notify");
        success = success && PROCESS_BOOL(root, &config->notify.smtp, use_tls, "Notify.SMTP.UseTLS", "Notify");
        success = success && PROCESS_INT(root, &config->notify.smtp, timeout, "Notify.SMTP.Timeout", "Notify");
        success = success && PROCESS_INT(root, &config->notify.smtp, max_retries, "Notify.SMTP.MaxRetries", "Notify");
        success = success && PROCESS_STRING(root, &config->notify.smtp, from_address, "Notify.SMTP.FromAddress", "Notify");
    }

    // Validate the configuration if loaded successfully
    if (success) {
        success = (config_notify_validate(&config->notify) == 0);
    }

    return success;
}

// Initialize notification configuration with default values
int config_notify_init(NotifyConfig* config) {
    if (!config) {
        log_this("Config", "Notify config pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    // Set default values
    config->enabled = true;
    
    // Allocate and copy default notifier
    config->notifier = strdup("none");
    if (!config->notifier) {
        log_this("Config", "Failed to allocate notifier type", LOG_LEVEL_ERROR);
        return -1;
    }

    // Initialize SMTP config with defaults
    config->smtp.host = NULL;
    config->smtp.port = DEFAULT_SMTP_PORT;
    config->smtp.username = NULL;
    config->smtp.password = NULL;
    config->smtp.use_tls = DEFAULT_SMTP_TLS;
    config->smtp.timeout = DEFAULT_SMTP_TIMEOUT;
    config->smtp.max_retries = DEFAULT_SMTP_MAX_RETRIES;
    config->smtp.from_address = NULL;

    return 0;
}

// Free resources allocated for notification configuration
void config_notify_cleanup(NotifyConfig* config) {
    if (!config) {
        return;
    }

    // Free main notify strings
    if (config->notifier) {
        free(config->notifier);
        config->notifier = NULL;
    }

    // Free SMTP strings
    if (config->smtp.host) {
        free(config->smtp.host);
        config->smtp.host = NULL;
    }
    if (config->smtp.username) {
        free(config->smtp.username);
        config->smtp.username = NULL;
    }
    if (config->smtp.password) {
        free(config->smtp.password);
        config->smtp.password = NULL;
    }
    if (config->smtp.from_address) {
        free(config->smtp.from_address);
        config->smtp.from_address = NULL;
    }

    // Zero out the structure
    memset(config, 0, sizeof(NotifyConfig));
}

// Validate notification configuration values
int config_notify_validate(const NotifyConfig* config) {
    if (!config) {
        log_this("Config", "Notify config pointer is NULL", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate notifier type
    if (!config->notifier || !config->notifier[0]) {
        log_this("Config", "Invalid notifier type (must not be empty)", LOG_LEVEL_ERROR);
        return -1;
    }

    // If SMTP is configured, validate SMTP settings
    if (strcmp(config->notifier, "smtp") == 0) {
        // Host is required for SMTP
        if (!config->smtp.host || !config->smtp.host[0]) {
            log_this("Config", "SMTP host is required when using SMTP notifier", LOG_LEVEL_ERROR);
            return -1;
        }

        // Port must be valid
        if (config->smtp.port <= 0 || config->smtp.port > 65535) {
            log_this("Config", "Invalid SMTP port number", LOG_LEVEL_ERROR);
            return -1;
        }

        // Timeout must be positive
        if (config->smtp.timeout <= 0) {
            log_this("Config", "Invalid SMTP timeout value", LOG_LEVEL_ERROR);
            return -1;
        }

        // Max retries must be non-negative
        if (config->smtp.max_retries < 0) {
            log_this("Config", "Invalid SMTP max retries value", LOG_LEVEL_ERROR);
            return -1;
        }

        // From address is required
        if (!config->smtp.from_address || !config->smtp.from_address[0]) {
            log_this("Config", "SMTP from address is required", LOG_LEVEL_ERROR);
            return -1;
        }
    }

    return 0;
}