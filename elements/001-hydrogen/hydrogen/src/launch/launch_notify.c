/*
 * Launch Notify Subsystem
 * 
 * This module handles the initialization of the notify subsystem.
 * It provides functions for checking readiness and launching notification services.
 */

 // Global includes 
#include "../hydrogen.h"

// Local includes
#include "launch.h"

// Forward declarations for validation helpers
static bool validate_smtp_settings(const SMTPConfig* config, int* msg_count, const char** messages);
static bool validate_notifier_type(const NotifyConfig* config, int* msg_count, const char** messages);

// Check if the notify subsystem is ready to launch
LaunchReadiness check_notify_launch_readiness(void) {
    LaunchReadiness readiness = {0};
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(20 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    int msg_count = 0;
    
    // Add the subsystem name as the first message
    readiness.messages[msg_count++] = strdup("Notify");
    
    // Check configuration
    if (!app_config) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Configuration not loaded");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Configuration loaded");

    // Skip validation if notify is disabled
    if (!app_config->notify.enabled) {
        readiness.messages[msg_count++] = strdup("  Go:      Notify disabled, skipping validation");
        readiness.messages[msg_count++] = strdup("  Decide:  Go For Launch of Notify Subsystem");
        readiness.messages[msg_count] = NULL;
        readiness.ready = true;
        return readiness;
    }

    // Validate notifier type
    if (!validate_notifier_type(&app_config->notify, &msg_count, readiness.messages)) {
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }

    // If SMTP notifier is configured, validate SMTP settings
    if (app_config->notify.notifier && strcmp(app_config->notify.notifier, "SMTP") == 0) {
        if (!validate_smtp_settings(&app_config->notify.smtp, &msg_count, readiness.messages)) {
            readiness.messages[msg_count] = NULL;
            readiness.ready = false;
            return readiness;
        }
    }
    
    // All checks passed
    readiness.messages[msg_count++] = strdup("  Decide:  Go For Launch of Notify Subsystem");
    readiness.messages[msg_count] = NULL;
    readiness.ready = true;
    
    return readiness;
}

static bool validate_notifier_type(const NotifyConfig* config, int* msg_count, const char** messages) {
    if (!config->notifier || strlen(config->notifier) == 0) {
        messages[(*msg_count)++] = strdup("  No-Go:   Notifier type is required when notify is enabled");
        return false;
    }

    // Currently only support SMTP
    if (strcmp(config->notifier, "SMTP") != 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Unsupported notifier type: %s", config->notifier);
        messages[(*msg_count)++] = strdup(msg);
        return false;
    }

    messages[(*msg_count)++] = strdup("  Go:      Notifier type valid");
    return true;
}

static bool validate_smtp_settings(const SMTPConfig* config, int* msg_count, const char** messages) {
    if (!config->host || strlen(config->host) == 0) {
        messages[(*msg_count)++] = strdup("  No-Go:   SMTP host is required");
        return false;
    }

    if (config->port < MIN_SMTP_PORT || config->port > MAX_SMTP_PORT) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid SMTP port %d (must be between %d and %d)",
                config->port, MIN_SMTP_PORT, MAX_SMTP_PORT);
        messages[(*msg_count)++] = strdup(msg);
        return false;
    }

    if (config->timeout < MIN_SMTP_TIMEOUT || config->timeout > MAX_SMTP_TIMEOUT) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid SMTP timeout %d (must be between %d and %d)",
                config->timeout, MIN_SMTP_TIMEOUT, MAX_SMTP_TIMEOUT);
        messages[(*msg_count)++] = strdup(msg);
        return false;
    }

    if (config->max_retries < MIN_SMTP_RETRIES || config->max_retries > MAX_SMTP_RETRIES) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid SMTP max retries %d (must be between %d and %d)",
                config->max_retries, MIN_SMTP_RETRIES, MAX_SMTP_RETRIES);
        messages[(*msg_count)++] = strdup(msg);
        return false;
    }

    if (!config->from_address || strlen(config->from_address) == 0) {
        messages[(*msg_count)++] = strdup("  No-Go:   SMTP from address is required");
        return false;
    }

    messages[(*msg_count)++] = strdup("  Go:      SMTP settings valid");
    return true;
}

// Launch the notify subsystem
int launch_notify_subsystem(void) {
    // Skip initialization if notify is disabled
    if (!app_config->notify.enabled) {
        log_this("Notify", "Notify subsystem disabled", LOG_LEVEL_STATE);
        return 1;
    }
    
    // Initialize notification system based on configured notifier
    if (strcmp(app_config->notify.notifier, "SMTP") == 0) {
        log_this("Notify", "Initializing SMTP notification service", LOG_LEVEL_STATE);
        // Additional SMTP initialization as needed
    }
    
    return 1;
}
