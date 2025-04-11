/*
 * Notification configuration JSON parsing
 */

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
    bool using_defaults = !json_is_object(notify);
    
    log_config_section("Notify", using_defaults);
    
    if (!using_defaults) {
        // Enabled flag
        json_t* val = json_object_get(notify, "Enabled");
        config->notify.enabled = get_config_bool(val, false);
        log_config_item("Enabled", config->notify.enabled ? "true" : "false", !val, 0);
        
        // Notifier type
        val = json_object_get(notify, "Notifier");
        bool using_default_notifier = !json_is_string(val);
        if (!using_default_notifier) {
            const char* type = json_string_value(val);
            if (config->notify.notifier) {
                free(config->notify.notifier);
            }
            config->notify.notifier = strdup(type);
            log_config_item("Notifier", type, false, 0);
        } else {
            config->notify.notifier = strdup("none");
            log_config_item("Notifier", "none", true, 0);
        }
        
        // SMTP Configuration
        json_t* smtp = json_object_get(notify, "SMTP");
        if (json_is_object(smtp)) {
            log_config_item("SMTP", "Configured", false, 0);
            
            // SMTP Host
            val = json_object_get(smtp, "Host");
            if (json_is_string(val)) {
                const char* host = json_string_value(val);
                if (config->notify.smtp.host) {
                    free(config->notify.smtp.host);
                }
                config->notify.smtp.host = strdup(host);
                log_config_item("Host", host, false, 1);
            }
            
            // SMTP Port
            val = json_object_get(smtp, "Port");
            config->notify.smtp.port = get_config_int(val, DEFAULT_SMTP_PORT);
            log_config_item("Port", format_int_buffer(config->notify.smtp.port), !val, 1);
            
            // SMTP Username
            val = json_object_get(smtp, "Username");
            if (json_is_string(val)) {
                const char* username = json_string_value(val);
                if (config->notify.smtp.username) {
                    free(config->notify.smtp.username);
                }
                config->notify.smtp.username = strdup(username);
                log_config_sensitive_item("Username", username, false, 1);
            }
            
            // SMTP Password (sensitive)
            val = json_object_get(smtp, "Password");
            if (json_is_string(val)) {
                const char* password = json_string_value(val);
                if (config->notify.smtp.password) {
                    free(config->notify.smtp.password);
                }
                config->notify.smtp.password = strdup(password);
                log_config_sensitive_item("Password", "********", false, 1);
            }
            
            // SMTP TLS
            val = json_object_get(smtp, "UseTLS");
            config->notify.smtp.use_tls = get_config_bool(val, DEFAULT_SMTP_TLS);
            log_config_item("UseTLS", config->notify.smtp.use_tls ? "true" : "false", !val, 1);
        }
        
        // Validate configuration
        if (config_notify_validate(&config->notify) != 0) {
            log_config_item("Status", "Invalid configuration, using defaults", true, 0);
            config_notify_cleanup(&config->notify);
            config_notify_init(&config->notify);
        }
        
        return true;
    } else {
        // No notification configuration section
        log_config_item("Status", "Section missing, using defaults", true, 0);
        return true;
    }
}