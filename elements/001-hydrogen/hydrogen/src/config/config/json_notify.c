/*
 * Notification configuration JSON parsing
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <jansson.h>
#include <stdbool.h>
#include <string.h>
#include "../../logging/logging.h"
#include "../config.h"
#include "../config_utils.h"
#include "../notify/config_notify.h"
#include "json_notify.h"

bool load_json_notify(json_t* root, AppConfig* config) {
    // First ensure any existing notification config is cleaned up
    config_notify_cleanup(&config->notify);
    
    // Initialize with defaults
    if (config_notify_init(&config->notify) != 0) {
        log_this("Config", "Failed to initialize notification configuration", LOG_LEVEL_ERROR);
        return false;
    }
    
    // Notification Configuration
    json_t* notify = json_object_get(root, "Notify");
    if (json_is_object(notify)) {
        log_config_section_header("Notify");
        
        // Enabled flag
        json_t* val = json_object_get(notify, "Enabled");
        config->notify.enabled = get_config_bool(val, false);
        log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !val, 0, NULL, NULL, "Config", 
                             config->notify.enabled ? "true" : "false");
        
        // Notifier type
        val = json_object_get(notify, "Notifier");
        if (json_is_string(val)) {
            const char* type = json_string_value(val);
            // Free existing value if any
            if (config->notify.notifier) {
                free(config->notify.notifier);
            }
            config->notify.notifier = strdup(type);
            log_config_section_item("Notifier", "%s", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config", type);
        } else {
            config->notify.notifier = strdup("none");
            log_config_section_item("Notifier", "%s", LOG_LEVEL_STATE, 1, 0, NULL, NULL, "Config", "none");
        }
        
        // SMTP Configuration
        json_t* smtp = json_object_get(notify, "SMTP");
        if (json_is_object(smtp)) {
            log_config_section_item("SMTP", "Configured", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            // SMTP Host
            val = json_object_get(smtp, "Host");
            if (json_is_string(val)) {
                const char* host = json_string_value(val);
                // Free existing value if any
                if (config->notify.smtp.host) {
                    free(config->notify.smtp.host);
                }
                config->notify.smtp.host = strdup(host);
                log_config_section_item("Host", "%s", LOG_LEVEL_STATE, 0, 1, NULL, NULL, "Config", host);
            }
            
            // SMTP Port
            val = json_object_get(smtp, "Port");
            config->notify.smtp.port = get_config_int(val, DEFAULT_SMTP_PORT);
            log_config_section_item("Port", "%d", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config", config->notify.smtp.port);
            
            // SMTP Username
            val = json_object_get(smtp, "Username");
            if (json_is_string(val)) {
                const char* username = json_string_value(val);
                // Free existing value if any
                if (config->notify.smtp.username) {
                    free(config->notify.smtp.username);
                }
                config->notify.smtp.username = strdup(username);
                log_config_section_item("Username", "%s", LOG_LEVEL_STATE, 0, 1, NULL, NULL, "Config", username);
            }
            
            // SMTP Password (sensitive)
            val = json_object_get(smtp, "Password");
            if (json_is_string(val)) {
                const char* password = json_string_value(val);
                // Free existing value if any
                if (config->notify.smtp.password) {
                    free(config->notify.smtp.password);
                }
                config->notify.smtp.password = strdup(password);
                // Log obfuscated password for security
                log_config_section_item("Password", "********", LOG_LEVEL_STATE, 0, 1, NULL, NULL, "Config");
            }
            
            // SMTP TLS
            val = json_object_get(smtp, "UseTLS");
            config->notify.smtp.use_tls = get_config_bool(val, DEFAULT_SMTP_TLS);
            log_config_section_item("UseTLS", "%s", LOG_LEVEL_STATE, !val, 1, NULL, NULL, "Config", 
                                 config->notify.smtp.use_tls ? "true" : "false");
        }
        
        // Validate configuration
        if (config_notify_validate(&config->notify) != 0) {
            log_config_section_item("Status", "Invalid configuration, using defaults", LOG_LEVEL_ERROR, 1, 0, NULL, NULL, "Config");
            config_notify_cleanup(&config->notify);
            config_notify_init(&config->notify);
        }
        
        return true;
    } else {
        // No notification configuration section
        log_config_section_header("Notify");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL, "Config");
        return true;
    }
}