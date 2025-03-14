/*
 * Notify configuration JSON parsing include file
 *
 * IMPORTANT: This is not a standalone C file!
 * This file is meant to be included directly inside the load_config function
 * in config.c using:
 *     #include "config/json_notify.inc"
 * 
 * Required context (must be available in load_config):
 * - json_t* root: The root JSON object
 * - AppConfig* config: The configuration structure
 * - All helper functions (get_config_string_with_env, etc.)
 * - All required headers must be included in config.c
 */

// Notify Configuration
json_t* notify = json_object_get(root, "Notify");
if (json_is_object(notify)) {
    log_config_section_header("Notify");
    
    json_t* enabled = json_object_get(notify, "Enabled");
    config->notify.enabled = get_config_bool(enabled, false);
    log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL,
            config->notify.enabled ? "true" : "false");
    
    json_t* notifier = json_object_get(notify, "Notifier");
    config->notify.notifier = get_config_string_with_env("Notifier", notifier, "SMTP");
    log_config_section_item("Notifier", "%s", LOG_LEVEL_STATE, !notifier, 0, NULL, NULL,
            config->notify.notifier);
    
    // If SMTP notifier, load SMTP configuration
    if (strcmp(config->notify.notifier, "SMTP") == 0) {
        json_t* smtp = json_object_get(notify, "SMTP");
        if (json_is_object(smtp)) {
            log_config_section_item("SMTP", "", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            // Load SMTP settings
            json_t* host = json_object_get(smtp, "Host");
            config->notify.smtp.host = get_config_string_with_env("Host", host, NULL);
            log_config_section_item("Host", "%s", LOG_LEVEL_STATE, !host, 1, NULL, NULL,
                    config->notify.smtp.host ? config->notify.smtp.host : "(not set)");
            
            json_t* port = json_object_get(smtp, "Port");
            config->notify.smtp.port = get_config_int(port, DEFAULT_SMTP_PORT);
            char port_str[32];
            snprintf(port_str, sizeof(port_str), "%d", config->notify.smtp.port);
            log_config_section_item("Port", "%s", LOG_LEVEL_STATE, !port, 1, NULL, NULL, "Config",
                    port_str);
            
            json_t* username = json_object_get(smtp, "Username");
            config->notify.smtp.username = get_config_string_with_env("Username", username, NULL);
            log_config_section_item("Username", "%s", LOG_LEVEL_STATE, !username, 1, NULL, NULL,
                    config->notify.smtp.username ? config->notify.smtp.username : "(not set)");
            
            json_t* password = json_object_get(smtp, "Password");
            config->notify.smtp.password = get_config_string_with_env("Password", password, NULL);
            // Don't log password value for security
            
            json_t* use_tls = json_object_get(smtp, "UseTLS");
            config->notify.smtp.use_tls = get_config_bool(use_tls, DEFAULT_SMTP_TLS);
            log_config_section_item("UseTLS", "%s", LOG_LEVEL_STATE, !use_tls, 1, NULL, NULL,
                    config->notify.smtp.use_tls ? "true" : "false");
        } else {
            log_config_section_item("SMTP", "Missing configuration", LOG_LEVEL_ERROR, 1, 0, NULL, NULL, "Config");
            return NULL;
        }
    }
} else {
    // Using defaults
    log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL, "Config");
    
    if (config_notify_init(&config->notify) != 0) {
        log_this("Config", "Failed to initialize notify configuration", LOG_LEVEL_ERROR);
        return NULL;
    }
}