/*
 * Notify Configuration Implementation
 *
 * Implements the configuration handlers for the notification subsystem,
 * including JSON parsing and environment variable handling.
 */

 // Global includes 
#include <src/hydrogen.h>

// Local includes
#include "config_notify.h"

// Load notification configuration from JSON
bool load_notify_config(json_t* root, AppConfig* config) {
    bool success = true;
    NotifyConfig* notify_config = &config->notify;

    // Zero out the config structure
    memset(notify_config, 0, sizeof(NotifyConfig));

    // Set secure defaults directly
    notify_config->enabled = true;
    notify_config->notifier = strdup("none");
    notify_config->smtp.port = 587;
    notify_config->smtp.use_tls = true;
    notify_config->smtp.timeout = 30;
    notify_config->smtp.max_retries = 3;

    // Process main notify section
    success = PROCESS_SECTION(root, "Notify");
    success = success && PROCESS_BOOL(root, notify_config, enabled, "Notify.Enabled", "Notify");
    success = success && PROCESS_STRING(root, notify_config, notifier, "Notify.Notifier", "Notify");

    // Process SMTP subsection if present
    if (success) {
        success = PROCESS_SECTION(root, "Notify.SMTP");
        success = success && PROCESS_STRING(root, &notify_config->smtp, host, "Notify.SMTP.Host", "Notify");
        success = success && PROCESS_INT(root, &notify_config->smtp, port, "Notify.SMTP.Port", "Notify");
        success = success && PROCESS_SENSITIVE(root, &notify_config->smtp, username, "Notify.SMTP.Username", "Notify");
        success = success && PROCESS_SENSITIVE(root, &notify_config->smtp, password, "Notify.SMTP.Password", "Notify");
        success = success && PROCESS_BOOL(root, &notify_config->smtp, use_tls, "Notify.SMTP.UseTLS", "Notify");
        success = success && PROCESS_INT(root, &notify_config->smtp, timeout, "Notify.SMTP.Timeout", "Notify");
        success = success && PROCESS_INT(root, &notify_config->smtp, max_retries, "Notify.SMTP.MaxRetries", "Notify");
        success = success && PROCESS_STRING(root, &notify_config->smtp, from_address, "Notify.SMTP.FromAddress", "Notify");
    }

    return success;
}

// Free resources allocated for notification configuration
void cleanup_notify_config(NotifyConfig* config) {
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

// Dump notification configuration
void dump_notify_config(const NotifyConfig* config) {
    if (!config) {
        DUMP_TEXT("", "Cannot dump NULL notify config");
        return;
    }

    // Dump main notify settings
    DUMP_BOOL("Enabled", config->enabled);
    DUMP_STRING("Notifier", config->notifier);

    // Dump SMTP configuration if notifier is "smtp"
    if (config->notifier && strcmp(config->notifier, "smtp") == 0) {
        DUMP_TEXT("――", "SMTP Configuration");
        DUMP_STRING("―――― Host", config->smtp.host);
        DUMP_INT("―――― Port", config->smtp.port);
        DUMP_SECRET("―――― Username", config->smtp.username);
        DUMP_SECRET("―――― Password", config->smtp.password);
        DUMP_BOOL("―――― Use TLS", config->smtp.use_tls);
        DUMP_INT("―――― Timeout", config->smtp.timeout);
        DUMP_INT("―――― Max Retries", config->smtp.max_retries);
        DUMP_STRING("―――― From Address", config->smtp.from_address);
    }
}
