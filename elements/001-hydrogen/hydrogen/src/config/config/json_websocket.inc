/*
 * WebSocket configuration JSON parsing include file
 * 
 * IMPORTANT: This is not a standalone C file!
 * This file is meant to be included directly inside the load_config function
 * in config.c using:
 *     #include "config/json_websocket.inc"
 * 
 * Required context (must be available in load_config):
 * - json_t* root: The root JSON object
 * - AppConfig* config: The configuration structure
 * - All helper functions (get_config_string_with_env, etc.)
 * - All required headers must be included in config.c
 */

// WebSocket Configuration
json_t* websocket = json_object_get(root, "WebSocket");
if (json_is_object(websocket)) {
    log_config_section_header("WebSocket");

    // Initialize with defaults if not already done
    if (!config->websocket.initialized) {
        config->websocket.enabled = true;
        config->websocket.enable_ipv6 = false;
        config->websocket.port = DEFAULT_WEBSOCKET_PORT;
        config->websocket.key = strdup("${env.WEBSOCKET_KEY}");
        config->websocket.protocol = strdup("hydrogen-protocol");
        config->websocket.max_message_size = 10 * 1024 * 1024;
        config->websocket.exit_wait_seconds = 10;
        config->websocket.initialized = true;
    }

    // Basic Properties
    json_t* enabled = json_object_get(websocket, "Enabled");
    config->websocket.enabled = get_config_bool(enabled, config->websocket.enabled);
    log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL, "Config",
        config->websocket.enabled ? "true" : "false");

    if (config->websocket.enabled) {
        // IPv6 Support
        json_t* enable_ipv6 = json_object_get(websocket, "EnableIPv6");
        config->websocket.enable_ipv6 = get_config_bool(enable_ipv6, config->websocket.enable_ipv6);
        log_config_section_item("EnableIPv6", "%s", LOG_LEVEL_STATE, !enable_ipv6, 0, NULL, NULL, "Config",
            config->websocket.enable_ipv6 ? "true" : "false");

        // Port Configuration
        json_t* port = json_object_get(websocket, "Port");
        config->websocket.port = get_config_int(port, config->websocket.port);
        log_config_section_item("Port", "%d", LOG_LEVEL_STATE, !port, 0, NULL, NULL, "Config",
            config->websocket.port);

        // Security Settings
        json_t* key = json_object_get(websocket, "Key");
        char* new_key = get_config_string_with_env("Key", key, config->websocket.key);
        if (new_key) {
            free(config->websocket.key);
            config->websocket.key = new_key;
        }
        // Note: Key value is sensitive, so it's handled by get_config_string_with_env's masking

        // Protocol Settings (with legacy support)
        json_t* protocol = json_object_get(websocket, "Protocol");
        if (!protocol) {
            protocol = json_object_get(websocket, "protocol");
            if (protocol) {
                log_this("Config", "Warning: Using legacy lowercase 'protocol' key, please update to 'Protocol'",
                    LOG_LEVEL_WARN);
            }
        }
        
        char* new_protocol = get_config_string_with_env("Protocol", protocol, config->websocket.protocol);
        if (new_protocol) {
            free(config->websocket.protocol);
            config->websocket.protocol = new_protocol;
            log_config_section_item("Protocol", "%s", LOG_LEVEL_STATE, !protocol, 0, NULL, NULL, "Config",
                config->websocket.protocol);
        } else {
            log_config_section_item("Protocol", "Failed to allocate string", LOG_LEVEL_ERROR, 1, 0, NULL, NULL, "Config");
            return NULL;
        }

        // Message Size Limits
        json_t* max_message_size = json_object_get(websocket, "MaxMessageSize");
        config->websocket.max_message_size = get_config_size(max_message_size, config->websocket.max_message_size);
        log_config_section_item("MaxMessageSize", "%zu", LOG_LEVEL_STATE, !max_message_size, 0, "B", "MB", "Config",
            config->websocket.max_message_size);

        // Connection Settings
        json_t* connection_timeouts = json_object_get(websocket, "ConnectionTimeouts");
        if (json_is_object(connection_timeouts)) {
            log_config_section_item("ConnectionTimeouts", "", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config");
            
            json_t* exit_wait_seconds = json_object_get(connection_timeouts, "ExitWaitSeconds");
            config->websocket.exit_wait_seconds = get_config_int(exit_wait_seconds, config->websocket.exit_wait_seconds);
            log_config_section_item("ExitWaitSeconds", "%d", LOG_LEVEL_STATE, !exit_wait_seconds, 1, NULL, NULL, "Config",
                config->websocket.exit_wait_seconds);
        }

        // Validate configuration
        if (config->websocket.max_message_size < 1024) {
            log_this("Config", "WebSocket MaxMessageSize must be at least 1KB", LOG_LEVEL_ERROR);
            return NULL;
        }
        if (config->websocket.exit_wait_seconds < 1) {
            log_this("Config", "WebSocket ExitWaitSeconds must be at least 1 second", LOG_LEVEL_ERROR);
            return NULL;
        }
    }
} else {
    log_config_section_header("WebSocket");
    log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL, "Config");
    
    // Initialize with defaults if not already done
    if (!config->websocket.initialized) {
        config->websocket.enabled = true;
        config->websocket.enable_ipv6 = false;
        config->websocket.port = DEFAULT_WEBSOCKET_PORT;
        config->websocket.key = strdup("${env.WEBSOCKET_KEY}");
        config->websocket.protocol = strdup("hydrogen-protocol");
        config->websocket.max_message_size = 10 * 1024 * 1024;
        config->websocket.exit_wait_seconds = 10;
        config->websocket.initialized = true;
    }
}