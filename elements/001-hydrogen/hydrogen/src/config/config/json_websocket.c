/*
 * WebSocketServer configuration JSON parsing
 */

// Core system headers
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Project headers
#include "json_websocket.h"
#include "../config.h"
#include "../env/config_env.h"
#include "../config_utils.h"
#include "../types/config_string.h"
#include "../types/config_bool.h"
#include "../types/config_int.h"
#include "../types/config_size.h"
#include "../config_defaults.h"
#include "../../logging/logging.h"
#include "../../utils/utils.h"
#include "../logging/config_logging_utils.h"

bool load_json_websocket(json_t* root, AppConfig* config) {
    // WebSocketServer Configuration
    json_t* websocket = json_object_get(root, "WebSocketServer");
    if (json_is_object(websocket)) {
        log_config_section_header("WebSocketServer");

        // Initialize defaults if this is the first time
        if (config->websocket.key == NULL) {
            config->websocket.enabled = DEFAULT_WEBSOCKET_ENABLED;
            config->websocket.enable_ipv6 = DEFAULT_WEBSOCKET_ENABLE_IPV6;
            config->websocket.port = DEFAULT_WEBSOCKET_PORT;
            config->websocket.key = strdup("${env.WEBSOCKET_KEY}");
            config->websocket.protocol = strdup("hydrogen-protocol");
            config->websocket.max_message_size = DEFAULT_MAX_MESSAGE_SIZE;
            config->websocket.exit_wait_seconds = DEFAULT_EXIT_WAIT_SECONDS;
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
                        LOG_LEVEL_ALERT);
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
                return false;
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
                log_this("Config", "WebSocketServer MaxMessageSize must be at least 1KB", LOG_LEVEL_ERROR);
                return false;
            }
            if (config->websocket.exit_wait_seconds < 1) {
                log_this("Config", "WebSocketServer ExitWaitSeconds must be at least 1 second", LOG_LEVEL_ERROR);
                return false;
            }
        }
    } else {
        log_config_section_header("WebSocketServer");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_ALERT, 1, 0, NULL, NULL, "Config");
        
        // Initialize with defaults
        config->websocket.enabled = DEFAULT_WEBSOCKET_ENABLED;
        config->websocket.enable_ipv6 = DEFAULT_WEBSOCKET_ENABLE_IPV6;
        config->websocket.port = DEFAULT_WEBSOCKET_PORT;
        
        // Only allocate strings if they don't already exist
        if (config->websocket.key == NULL) {
            config->websocket.key = strdup("${env.WEBSOCKET_KEY}");
        }
        if (config->websocket.protocol == NULL) {
            config->websocket.protocol = strdup("hydrogen-protocol");
        }
        
        config->websocket.max_message_size = DEFAULT_MAX_MESSAGE_SIZE;
        config->websocket.exit_wait_seconds = DEFAULT_EXIT_WAIT_SECONDS;
    }
    
    return true;
}