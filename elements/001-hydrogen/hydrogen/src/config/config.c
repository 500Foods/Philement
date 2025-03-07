/*
 * Configuration management system with robust fallback handling
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <sys/types.h>
#include <time.h>
#include <errno.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Project headers
#include "config.h"
#include "config_env.h"
#include "config_string.h"
#include "config_bool.h"
#include "config_int.h"
#include "config_size.h"
#include "config_double.h"
#include "config_filesystem.h"
#include "config_priority.h"
#include "config_defaults.h"

// Subsystem headers
#include "config_webserver.h"
#include "config_websocket.h"
#include "config_network.h"
#include "config_monitoring.h"
#include "config_print_queue.h"
#include "config_oidc.h"
#include "config_resources.h"
#include "config_mdns.h"
#include "config_logging.h"
#include "config_api.h"

#include "../logging/logging.h"
#include "../utils/utils.h"


// Global static configuration instance
static AppConfig *app_config = NULL;

/*
 * Helper function to detect sensitive configuration values
 * 
 * This function checks if a configuration key contains sensitive terms
 * like "key", "token", "pass", etc. that might contain secrets.
 * 
 * @param name The configuration key name
 * @return true if the name contains a sensitive term, false otherwise
 */
static bool is_sensitive_value(const char* name) {
    if (!name) return false;
    
    const char* sensitive_terms[] = {
        "key", "token", "pass", "seed", "jwt", "secret"
    };
    int num_terms = sizeof(sensitive_terms) / sizeof(sensitive_terms[0]);
    
    for (int i = 0; i < num_terms; i++) {
        if (strcasestr(name, sensitive_terms[i])) {
            return true;
        }
    }
    
    return false;
}
    

/*
 * Format a configuration value, truncating sensitive string values
 * 
 * This function formats a configuration value and truncates sensitive strings
 * to 5 characters to avoid logging secrets in full.
 * 
 * @param buffer Output buffer
 * @param buffer_size Size of the output buffer
 * @param name Configuration key name
 * @param format Format string
 * @param args Variable arguments
 */
void format_config_value(char* buffer, size_t buffer_size, const char* name, const char* format, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);
    
    /* Create the formatted value first */
    char formatted_value[1024] = {0};
    vsnprintf(formatted_value, sizeof(formatted_value), format, args_copy);
    va_end(args_copy);
    
    /* For sensitive string values, truncate */
    if (is_sensitive_value(name) && strstr(format, "%s")) {
        int len = strlen(formatted_value);
        if (len > 5) {
            formatted_value[5] = '\0';
            strncat(formatted_value, "...", sizeof(formatted_value) - strlen(formatted_value) - 1);
        }
    }
    
    strncpy(buffer, formatted_value, buffer_size);
    buffer[buffer_size - 1] = '\0';
}

/*
 * Helper function to log a configuration section header
 * 
 * This function logs a section header with the name of the configuration section.
 * It's used to group related configuration items in the log.
 * 
 * @param section_name The name of the configuration section
 */
void log_config_section_header(const char* section_name) {
    log_this("Config", "%s", LOG_LEVEL_INFO, section_name);
}

/*
 * Helper function to log a regular configuration item
 * 
 * This function logs a configuration item with proper indentation to show
 * it belongs to a section. The format is "- key: value"
 * 
 * @param key The configuration key
 * @param format The format string for the value
 * @param level The log level
 * @param ... Variable arguments for the format string
 */
void log_config_section_item(const char* key, const char* format, int level, ...) {
    if (!key || !format) {
        return;
    }
    
    va_list args;
    va_start(args, level);
    
    char value_buffer[64] = {0};
    vsnprintf(value_buffer, sizeof(value_buffer), format, args);
    va_end(args);
    
    char message[1024] = {0};
    strncat(message, "- ", sizeof(message) - strlen(message) - 1);
    strncat(message, key, sizeof(message) - strlen(message) - 1);
    strncat(message, ": ", sizeof(message) - strlen(message) - 1);
    strncat(message, value_buffer, sizeof(message) - strlen(message) - 1);
    
    log_this("Config", "%s", level, message);
}



/*
 * Get the current application configuration
 * 
 * Returns a pointer to the current application configuration.
 * This configuration is loaded by load_config() and stored in a static variable.
 * The returned pointer should not be modified by the caller.
 */
const AppConfig* get_app_config(void) {
    return app_config;
}

/*
 * Load and validate configuration with comprehensive error handling
 */
AppConfig* load_config(const char* config_path) {

    log_this("Config", "%s", LOG_LEVEL_INFO, LOG_LINE_BREAK);
    log_this("Config", "CONFIGURATION", LOG_LEVEL_INFO);

    // Free previous configuration if it exists
    if (app_config) {
        free(app_config);
    }

    json_error_t error;
    json_t* root = json_load_file(config_path, 0, &error);

    if (!root) {
        log_this("Config", "Failed to load config file: %s (line %d, column %d)",
                 LOG_LEVEL_ERROR, error.text, error.line, error.column);
        
        fprintf(stderr, "ERROR: Hydrogen configuration file has JSON syntax errors.\n");
        fprintf(stderr, "       %s at line %d, column %d\n", error.text, error.line, error.column);
        fprintf(stderr, "       Please fix the syntax error and try again.\n");
        
        exit(EXIT_FAILURE);
    }

    AppConfig* config = calloc(1, sizeof(AppConfig));
    if (!config) {
        log_this("Config", "Failed to allocate memory for config", LOG_LEVEL_DEBUG);
        json_decref(root);
        return NULL;
    }
    
    // Initialize critical defaults that must be set regardless of JSON presence
    config->web.port = DEFAULT_WEB_PORT;
    config->websocket.port = DEFAULT_WEBSOCKET_PORT;
    config->server.startup_delay = DEFAULT_STARTUP_DELAY;
    app_config = config;

    // Store paths
    config->config_file = strdup(config_path);  // Store the config file path
    config->executable_path = get_executable_path();
    if (!config->executable_path) {
        log_this("Config", "Failed to get executable path, using default", LOG_LEVEL_INFO);
        config->executable_path = strdup("./hydrogen");
    }

    // Server Configuration
    json_t* server = json_object_get(root, "Server");
    if (json_is_object(server)) {
        log_config_section_header("Server");
        
        // Server Name
        json_t* server_name = json_object_get(server, "ServerName");
        config->server.server_name = get_config_string(server_name, DEFAULT_SERVER_NAME);
        if (json_is_string(server_name) && 
            strncmp(json_string_value(server_name), "${env.", 6) == 0 &&
            !getenv(json_string_value(server_name) + 6)) {
            log_config_section_item("ServerName", "(default)", LOG_LEVEL_INFO);
        } else {
            log_config_section_item("ServerName", "%s", LOG_LEVEL_INFO, config->server.server_name);
        }
        
        // Payload Key (for payload decryption)
        json_t* payload_key = json_object_get(server, "PayloadKey");
        config->server.payload_key = get_config_string(payload_key, "${env.PAYLOAD_KEY}");
        if (config->server.payload_key && strncmp(config->server.payload_key, "${env.", 6) == 0) {
            const char* env_var = config->server.payload_key + 6;
            size_t env_var_len = strlen(env_var);
            if (env_var_len > 1 && env_var[env_var_len - 1] == '}') {
                char var_name[256] = {0};
                strncpy(var_name, env_var, env_var_len - 1);
                var_name[env_var_len - 1] = '\0';
                
                const char* env_value = getenv(var_name);
                if (env_value) {
                    char safe_value[256];
                    strncpy(safe_value, env_value, 5);
                    safe_value[5] = '\0';
                    strcat(safe_value, "...");
                    log_this("Config-Env", "- PayloadKey: $%s: %s", LOG_LEVEL_INFO, var_name, safe_value);
                } else {
                    log_this("Config-Env", "- PayloadKey: $%s: (not set)", LOG_LEVEL_INFO, var_name);
                }
            }
        } else if (config->server.payload_key) {
            log_this("Config", "- PayloadKey: %s", LOG_LEVEL_INFO, config->server.payload_key);
        }

        // Log File
        json_t* log_file = json_object_get(server, "LogFile");
        config->server.log_file_path = get_config_string(log_file, DEFAULT_LOG_FILE_PATH);
        log_config_section_item("LogFile", "%s", LOG_LEVEL_INFO, config->server.log_file_path);

        // Startup Delay (in milliseconds)
        json_t* startup_delay = json_object_get(server, "StartupDelay");
        config->server.startup_delay = get_config_int(startup_delay, DEFAULT_STARTUP_DELAY);
        log_config_section_item("StartupDelay", "%d", LOG_LEVEL_INFO, config->server.startup_delay);
    } else {
        // Fallback to defaults if Server object is missing
        config->server.server_name = strdup(DEFAULT_SERVER_NAME);
        config->server.payload_key = strdup("${env.PAYLOAD_KEY}");
        config->server.log_file_path = strdup(DEFAULT_LOG_FILE_PATH);
        config->server.startup_delay = DEFAULT_STARTUP_DELAY;
        log_config_section_header("Server");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_WARN);
        log_config_section_item("ServerName", "%s", LOG_LEVEL_INFO, DEFAULT_SERVER_NAME);
        log_config_section_item("PayloadKey", "${env.PAYLOAD_KEY}", LOG_LEVEL_INFO);
        log_config_section_item("LogFile", "%s", LOG_LEVEL_INFO, DEFAULT_LOG_FILE_PATH);
        log_config_section_item("StartupDelay", "%d", LOG_LEVEL_INFO, DEFAULT_STARTUP_DELAY);
    }

    // Web Configuration
    json_t* web = json_object_get(root, "WebServer");
    if (json_is_object(web)) {
        log_config_section_header("WebServer");
        
        json_t* enabled = json_object_get(web, "Enabled");
        config->web.enabled = get_config_bool(enabled, 1);
        log_config_section_item("Enabled", "%s", LOG_LEVEL_INFO, 
                config->web.enabled ? "true" : "false");

        json_t* enable_ipv6 = json_object_get(web, "EnableIPv6");
        config->web.enable_ipv6 = get_config_bool(enable_ipv6, 0);
        log_config_section_item("EnableIPv6", "%s", LOG_LEVEL_INFO, 
                config->web.enable_ipv6 ? "true" : "false");

        json_t* port = json_object_get(web, "Port");
        config->web.port = json_is_integer(port) ? json_integer_value(port) : DEFAULT_WEB_PORT;
        log_config_section_item("Port", "%d", LOG_LEVEL_INFO, config->web.port);

        json_t* web_root = json_object_get(web, "WebRoot");
        config->web.web_root = get_config_string(web_root, "/var/www/html");
        log_config_section_item("WebRoot", "%s", LOG_LEVEL_INFO, config->web.web_root);

        json_t* upload_path = json_object_get(web, "UploadPath");
        config->web.upload_path = get_config_string(upload_path, DEFAULT_UPLOAD_PATH);
        log_config_section_item("UploadPath", "%s", LOG_LEVEL_INFO, config->web.upload_path);

        json_t* upload_dir = json_object_get(web, "UploadDir");
        config->web.upload_dir = get_config_string(upload_dir, DEFAULT_UPLOAD_DIR);
        log_config_section_item("UploadDir", "%s", LOG_LEVEL_INFO, config->web.upload_dir);

        json_t* max_upload_size = json_object_get(web, "MaxUploadSize");
        config->web.max_upload_size = get_config_size(max_upload_size, DEFAULT_MAX_UPLOAD_SIZE);
        log_config_section_item("MaxUploadSize", "%zu bytes", LOG_LEVEL_INFO, config->web.max_upload_size);
        
        json_t* api_prefix = json_object_get(web, "ApiPrefix");
        config->web.api_prefix = get_config_string(api_prefix, "/api");
        log_config_section_item("ApiPrefix", "%s", LOG_LEVEL_INFO, config->web.api_prefix);
    } else {
        config->web.port = DEFAULT_WEB_PORT;
        config->web.web_root = strdup("/var/www/html");
        config->web.upload_path = strdup(DEFAULT_UPLOAD_PATH);
        config->web.upload_dir = strdup(DEFAULT_UPLOAD_DIR);
        config->web.max_upload_size = DEFAULT_MAX_UPLOAD_SIZE;
        config->web.api_prefix = strdup("/api");
        log_config_section_header("WebServer");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_WARN);
        log_config_section_item("Enabled", "true", LOG_LEVEL_INFO);
        log_config_section_item("Port", "%d", LOG_LEVEL_INFO, DEFAULT_WEB_PORT);
        log_config_section_item("ApiPrefix", "%s", LOG_LEVEL_INFO, config->web.api_prefix);
    }

    // WebSocket Configuration
    json_t* websocket = json_object_get(root, "WebSocket");
    if (json_is_object(websocket)) {
        log_config_section_header("WebSocket");
        
        json_t* enabled = json_object_get(websocket, "Enabled");
        config->websocket.enabled = get_config_bool(enabled, 1);
        log_config_section_item("Enabled", "%s", LOG_LEVEL_INFO,
                config->websocket.enabled ? "true" : "false");

        json_t* enable_ipv6 = json_object_get(websocket, "EnableIPv6");
        config->websocket.enable_ipv6 = get_config_bool(enable_ipv6, 0);
        log_config_section_item("EnableIPv6", "%s", LOG_LEVEL_INFO,
                config->websocket.enable_ipv6 ? "true" : "false");

        json_t* port = json_object_get(websocket, "Port");
        config->websocket.port = json_is_integer(port) ? json_integer_value(port) : DEFAULT_WEBSOCKET_PORT;
        log_config_section_item("Port", "%d", LOG_LEVEL_INFO, config->websocket.port);

        json_t* key = json_object_get(websocket, "Key");
        config->websocket.key = get_config_string(key, "default_key");
        if (config->websocket.key && strncmp(config->websocket.key, "${env.", 6) == 0) {
            const char* env_var = config->websocket.key + 6;
            size_t env_var_len = strlen(env_var);
            if (env_var_len > 1 && env_var[env_var_len - 1] == '}') {
                char var_name[256] = {0};
                strncpy(var_name, env_var, env_var_len - 1);
                var_name[env_var_len - 1] = '\0';
                
                const char* env_value = getenv(var_name);
                if (env_value) {
                    char safe_value[256];
                    strncpy(safe_value, env_value, 5);
                    safe_value[5] = '\0';
                    strcat(safe_value, "...");
                    log_this("Config-Env", "- Key: $%s: %s", LOG_LEVEL_INFO, var_name, safe_value);
                } else {
                    log_this("Config-Env", "- Key: $%s: (not set)", LOG_LEVEL_INFO, var_name);
                }
            }
        } else if (config->websocket.key) {
            log_this("Config", "- Key: %s", LOG_LEVEL_INFO, config->websocket.key);
        }

        json_t* protocol = json_object_get(websocket, "protocol");
        if (!protocol) {
            protocol = json_object_get(websocket, "Protocol");  // Try legacy uppercase key
        }
        config->websocket.protocol = get_config_string(protocol, "hydrogen-protocol");
        if (!config->websocket.protocol) {
            log_config_section_item("Protocol", "Failed to allocate string", LOG_LEVEL_ERROR);
            free(config->websocket.key);
            return NULL;
        }
        log_config_section_item("Protocol", "%s", LOG_LEVEL_INFO, config->websocket.protocol);

        json_t* max_message_size = json_object_get(websocket, "MaxMessageSize");
        config->websocket.max_message_size = get_config_size(max_message_size, 10 * 1024 * 1024);
        log_config_section_item("MaxMessageSize", "%zu bytes", LOG_LEVEL_INFO, config->websocket.max_message_size);

        json_t* connection_timeouts = json_object_get(websocket, "ConnectionTimeouts");
        if (json_is_object(connection_timeouts)) {
            log_config_section_item("ConnectionTimeouts", "Configured", LOG_LEVEL_INFO);
            
            json_t* exit_wait_seconds = json_object_get(connection_timeouts, "ExitWaitSeconds");
            config->websocket.exit_wait_seconds = get_config_int(exit_wait_seconds, 10);
            log_config_section_item("ExitWaitSeconds", "%d", LOG_LEVEL_INFO, config->websocket.exit_wait_seconds);
        } else {
            config->websocket.exit_wait_seconds = 10;
        }
    } else {
        config->websocket.port = DEFAULT_WEBSOCKET_PORT;
        config->websocket.key = strdup("default_key");
        config->websocket.protocol = strdup("hydrogen-protocol");
        config->websocket.max_message_size = 10 * 1024 * 1024;
        config->websocket.exit_wait_seconds = 10;
        
        log_config_section_header("WebSocket");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_WARN);
        log_config_section_item("Enabled", "true", LOG_LEVEL_INFO);
        log_config_section_item("Port", "%d", LOG_LEVEL_INFO, DEFAULT_WEBSOCKET_PORT);
        log_config_section_item("Protocol", "%s", LOG_LEVEL_INFO, "hydrogen-protocol");
    }

    // mDNS Server Configuration
    json_t* mdns_server = json_object_get(root, "mDNSServer");
    if (json_is_object(mdns_server)) {
        log_config_section_header("mDNSServer");
        
        json_t* enabled = json_object_get(mdns_server, "Enabled");
        config->mdns_server.enabled = get_config_bool(enabled, 1);
        log_config_section_item("Enabled", "%s", LOG_LEVEL_INFO, 
                 config->mdns_server.enabled ? "true" : "false");

        json_t* enable_ipv6 = json_object_get(mdns_server, "EnableIPv6");
        config->mdns_server.enable_ipv6 = get_config_bool(enable_ipv6, 1);
        log_config_section_item("EnableIPv6", "%s", LOG_LEVEL_INFO, 
                 config->mdns_server.enable_ipv6 ? "true" : "false");

        json_t* device_id = json_object_get(mdns_server, "DeviceId");
        config->mdns_server.device_id = get_config_string(device_id, "hydrogen-printer");
        log_config_section_item("DeviceId", "%s", LOG_LEVEL_INFO, config->mdns_server.device_id);

        json_t* friendly_name = json_object_get(mdns_server, "FriendlyName");
        config->mdns_server.friendly_name = get_config_string(friendly_name, "Hydrogen 3D Printer");
        log_config_section_item("FriendlyName", "%s", LOG_LEVEL_INFO, config->mdns_server.friendly_name);

        json_t* model = json_object_get(mdns_server, "Model");
        config->mdns_server.model = get_config_string(model, "Hydrogen");
        log_config_section_item("Model", "%s", LOG_LEVEL_INFO, config->mdns_server.model);

        json_t* manufacturer = json_object_get(mdns_server, "Manufacturer");
        config->mdns_server.manufacturer = get_config_string(manufacturer, "Philement");
        log_config_section_item("Manufacturer", "%s", LOG_LEVEL_INFO, config->mdns_server.manufacturer);

        json_t* version = json_object_get(mdns_server, "Version");
        config->mdns_server.version = get_config_string(version, VERSION);
        log_config_section_item("Version", "%s", LOG_LEVEL_INFO, config->mdns_server.version);
        
        json_t* services = json_object_get(mdns_server, "Services");
        if (json_is_array(services)) {
            config->mdns_server.num_services = json_array_size(services);
            log_config_section_item("Services", "%zu configured", LOG_LEVEL_INFO, config->mdns_server.num_services);
            config->mdns_server.services = calloc(config->mdns_server.num_services, sizeof(mdns_server_service_t));
            for (size_t i = 0; i < config->mdns_server.num_services; i++) {
                json_t* service = json_array_get(services, i);
                if (!json_is_object(service)) continue;
                // Get service properties
                json_t* name = json_object_get(service, "Name");
                config->mdns_server.services[i].name = get_config_string(name, "hydrogen");
                
                json_t* type = json_object_get(service, "Type");
                config->mdns_server.services[i].type = get_config_string(type, "_http._tcp.local");

                json_t* port = json_object_get(service, "Port");
                config->mdns_server.services[i].port = get_config_int(port, DEFAULT_WEB_PORT);
                
                // Log service details after all properties are populated
                log_config_section_item("Service", "%s: %s on port %d", LOG_LEVEL_INFO, 
                                       config->mdns_server.services[i].name,
                                       config->mdns_server.services[i].type,
                                       config->mdns_server.services[i].port);

                json_t* txt_records = json_object_get(service, "TxtRecords");
                if (json_is_string(txt_records)) {
                    config->mdns_server.services[i].num_txt_records = 1;
                    config->mdns_server.services[i].txt_records = malloc(sizeof(char*));
                    config->mdns_server.services[i].txt_records[0] = get_config_string(txt_records, "");
                } else if (json_is_array(txt_records)) {
                    config->mdns_server.services[i].num_txt_records = json_array_size(txt_records);
                    config->mdns_server.services[i].txt_records = malloc(config->mdns_server.services[i].num_txt_records * sizeof(char*));
                    for (size_t j = 0; j < config->mdns_server.services[i].num_txt_records; j++) {
                        json_t* record = json_array_get(txt_records, j);
                        config->mdns_server.services[i].txt_records[j] = get_config_string(record, "");
                    }
                } else {
                    config->mdns_server.services[i].num_txt_records = 0;
                    config->mdns_server.services[i].txt_records = NULL;
                }
            }
        }
    } else {
        log_config_section_header("mDNSServer");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_WARN);
    }

    // System Resources Configuration
    json_t* resources = json_object_get(root, "SystemResources");
    if (json_is_object(resources)) {
        log_config_section_header("SystemResources");
        
        json_t* queues = json_object_get(resources, "Queues");
        if (json_is_object(queues)) {
            log_config_section_item("Queues", "Configured", LOG_LEVEL_INFO);
            
            json_t* val;
            val = json_object_get(queues, "MaxQueueBlocks");
            config->resources.max_queue_blocks = get_config_size(val, DEFAULT_MAX_QUEUE_BLOCKS);
            log_config_section_item("MaxQueueBlocks", "%zu", LOG_LEVEL_INFO, config->resources.max_queue_blocks);
            
            val = json_object_get(queues, "QueueHashSize");
            config->resources.queue_hash_size = get_config_size(val, DEFAULT_QUEUE_HASH_SIZE);
            log_config_section_item("QueueHashSize", "%zu", LOG_LEVEL_INFO, config->resources.queue_hash_size);
            
            val = json_object_get(queues, "DefaultQueueCapacity");
            config->resources.default_capacity = get_config_size(val, DEFAULT_QUEUE_CAPACITY);
            log_config_section_item("DefaultQueueCapacity", "%zu", LOG_LEVEL_INFO, config->resources.default_capacity);
        }

        json_t* buffers = json_object_get(resources, "Buffers");
        if (json_is_object(buffers)) {
            log_config_section_item("Buffers", "Configured", LOG_LEVEL_INFO);
            
            json_t* val;
            val = json_object_get(buffers, "DefaultMessageBuffer");
            config->resources.message_buffer_size = get_config_size(val, DEFAULT_MESSAGE_BUFFER_SIZE);
            log_config_section_item("DefaultMessageBuffer", "%zu bytes", LOG_LEVEL_INFO, config->resources.message_buffer_size);
            
            val = json_object_get(buffers, "MaxLogMessageSize");
            config->resources.max_log_message_size = get_config_size(val, DEFAULT_MAX_LOG_MESSAGE_SIZE);
            log_config_section_item("MaxLogMessageSize", "%zu bytes", LOG_LEVEL_INFO, config->resources.max_log_message_size);
            
            val = json_object_get(buffers, "LineBufferSize");
            config->resources.line_buffer_size = get_config_size(val, DEFAULT_LINE_BUFFER_SIZE);
            log_config_section_item("LineBufferSize", "%zu bytes", LOG_LEVEL_INFO, config->resources.line_buffer_size);

            val = json_object_get(buffers, "PostProcessorBuffer");
            config->resources.post_processor_buffer_size = get_config_size(val, DEFAULT_POST_PROCESSOR_BUFFER_SIZE);
            log_config_section_item("PostProcessorBuffer", "%zu bytes", LOG_LEVEL_INFO, config->resources.post_processor_buffer_size);

            val = json_object_get(buffers, "LogBufferSize");
            config->resources.log_buffer_size = get_config_size(val, DEFAULT_LOG_BUFFER_SIZE);
            log_config_section_item("LogBufferSize", "%zu bytes", LOG_LEVEL_INFO, config->resources.log_buffer_size);

            val = json_object_get(buffers, "JsonMessageSize");
            config->resources.json_message_size = get_config_size(val, DEFAULT_JSON_MESSAGE_SIZE);
            log_config_section_item("JsonMessageSize", "%zu bytes", LOG_LEVEL_INFO, config->resources.json_message_size);

            val = json_object_get(buffers, "LogEntrySize");
            config->resources.log_entry_size = get_config_size(val, DEFAULT_LOG_ENTRY_SIZE);
            log_config_section_item("LogEntrySize", "%zu bytes", LOG_LEVEL_INFO, config->resources.log_entry_size);
        }
    } else {
        config->resources.max_queue_blocks = DEFAULT_MAX_QUEUE_BLOCKS;
        config->resources.queue_hash_size = DEFAULT_QUEUE_HASH_SIZE;
        config->resources.default_capacity = DEFAULT_QUEUE_CAPACITY;
        config->resources.message_buffer_size = DEFAULT_MESSAGE_BUFFER_SIZE;
        config->resources.max_log_message_size = DEFAULT_MAX_LOG_MESSAGE_SIZE;
        config->resources.line_buffer_size = DEFAULT_LINE_BUFFER_SIZE;
        config->resources.post_processor_buffer_size = DEFAULT_POST_PROCESSOR_BUFFER_SIZE;
        config->resources.log_buffer_size = DEFAULT_LOG_BUFFER_SIZE;
        config->resources.json_message_size = DEFAULT_JSON_MESSAGE_SIZE;
        config->resources.log_entry_size = DEFAULT_LOG_ENTRY_SIZE;
        
        log_config_section_header("SystemResources");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_WARN);
    }

    // Network Configuration
    json_t* network = json_object_get(root, "Network");
    if (json_is_object(network)) {
        log_config_section_header("Network");
        
        json_t* interfaces = json_object_get(network, "Interfaces");
        if (json_is_object(interfaces)) {
            log_config_section_item("Interfaces", "Configured", LOG_LEVEL_INFO);
            
            json_t* val;
            val = json_object_get(interfaces, "MaxInterfaces");
            config->network.max_interfaces = get_config_size(val, DEFAULT_MAX_INTERFACES);
            log_config_section_item("MaxInterfaces", "%zu", LOG_LEVEL_INFO, config->network.max_interfaces);
            
            val = json_object_get(interfaces, "MaxIPsPerInterface");
            config->network.max_ips_per_interface = get_config_size(val, DEFAULT_MAX_IPS_PER_INTERFACE);
            log_config_section_item("MaxIPsPerInterface", "%zu", LOG_LEVEL_INFO, config->network.max_ips_per_interface);
            
            val = json_object_get(interfaces, "MaxInterfaceNameLength");
            config->network.max_interface_name_length = get_config_size(val, DEFAULT_MAX_INTERFACE_NAME_LENGTH);
            log_config_section_item("MaxInterfaceNameLength", "%zu", LOG_LEVEL_INFO, config->network.max_interface_name_length);
            
            val = json_object_get(interfaces, "MaxIPAddressLength");
            config->network.max_ip_address_length = get_config_size(val, DEFAULT_MAX_IP_ADDRESS_LENGTH);
            log_config_section_item("MaxIPAddressLength", "%zu", LOG_LEVEL_INFO, config->network.max_ip_address_length);
        }

        json_t* port_allocation = json_object_get(network, "PortAllocation");
        if (json_is_object(port_allocation)) {
            log_config_section_item("PortAllocation", "Configured", LOG_LEVEL_INFO);
            
            json_t* val;
            val = json_object_get(port_allocation, "StartPort");
            config->network.start_port = get_config_int(val, DEFAULT_WEB_PORT);
            log_config_section_item("StartPort", "%d", LOG_LEVEL_INFO, config->network.start_port);
            
            val = json_object_get(port_allocation, "EndPort");
            config->network.end_port = get_config_int(val, 65535);
            log_config_section_item("EndPort", "%d", LOG_LEVEL_INFO, config->network.end_port);
                log_config_section_item("ReservedPorts", "%zu ports reserved", LOG_LEVEL_INFO, config->network.reserved_ports_count);
            json_t* reserved_ports = json_object_get(port_allocation, "ReservedPorts");
            if (json_is_array(reserved_ports)) {
                config->network.reserved_ports_count = json_array_size(reserved_ports);
                config->network.reserved_ports = malloc(sizeof(int) * config->network.reserved_ports_count);
                for (size_t i = 0; i < config->network.reserved_ports_count; i++) {
                    config->network.reserved_ports[i] = json_integer_value(json_array_get(reserved_ports, i));
                }
            }
        }
    } else {
        config->network.max_interfaces = DEFAULT_MAX_INTERFACES;
        config->network.max_ips_per_interface = DEFAULT_MAX_IPS_PER_INTERFACE;
        config->network.max_interface_name_length = DEFAULT_MAX_INTERFACE_NAME_LENGTH;
        config->network.max_ip_address_length = DEFAULT_MAX_IP_ADDRESS_LENGTH;
        config->network.start_port = DEFAULT_WEB_PORT;
        config->network.end_port = 65535;
        config->network.reserved_ports = NULL;
        config->network.reserved_ports_count = 0;
        
        log_config_section_header("Network");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_WARN);
    }

    // System Monitoring Configuration
    json_t* monitoring = json_object_get(root, "SystemMonitoring");
    if (json_is_object(monitoring)) {
        log_config_section_header("SystemMonitoring");
        
        json_t* intervals = json_object_get(monitoring, "Intervals");
        if (json_is_object(intervals)) {
            log_config_section_item("Intervals", "Configured", LOG_LEVEL_INFO);
            
            json_t* val;
            val = json_object_get(intervals, "StatusUpdateMs");
            config->monitoring.status_update_ms = get_config_size(val, DEFAULT_STATUS_UPDATE_MS);
            log_config_section_item("StatusUpdateMs", "%zu ms", LOG_LEVEL_INFO, config->monitoring.status_update_ms);
            
            val = json_object_get(intervals, "ResourceCheckMs");
            config->monitoring.resource_check_ms = get_config_size(val, DEFAULT_RESOURCE_CHECK_MS);
            log_config_section_item("ResourceCheckMs", "%zu ms", LOG_LEVEL_INFO, config->monitoring.resource_check_ms);
            
            val = json_object_get(intervals, "MetricsUpdateMs");
            config->monitoring.metrics_update_ms = get_config_size(val, DEFAULT_METRICS_UPDATE_MS);
            log_config_section_item("MetricsUpdateMs", "%zu ms", LOG_LEVEL_INFO, config->monitoring.metrics_update_ms);
        }

        json_t* thresholds = json_object_get(monitoring, "Thresholds");
        if (json_is_object(thresholds)) {
            log_config_section_item("Thresholds", "Configured", LOG_LEVEL_INFO);
            
            json_t* val;
            val = json_object_get(thresholds, "MemoryWarningPercent");
            config->monitoring.memory_warning_percent = get_config_int(val, DEFAULT_MEMORY_WARNING_PERCENT);
            log_config_section_item("MemoryWarningPercent", "%d%%", LOG_LEVEL_INFO, config->monitoring.memory_warning_percent);
            
            val = json_object_get(thresholds, "DiskSpaceWarningPercent");
            config->monitoring.disk_warning_percent = get_config_int(val, DEFAULT_DISK_WARNING_PERCENT);
            log_config_section_item("DiskSpaceWarningPercent", "%d%%", LOG_LEVEL_INFO, config->monitoring.disk_warning_percent);
            
            val = json_object_get(thresholds, "LoadAverageWarning");
            config->monitoring.load_warning = get_config_double(val, DEFAULT_LOAD_WARNING);
            log_config_section_item("LoadAverageWarning", "%.1f", LOG_LEVEL_INFO, config->monitoring.load_warning);
        }
    } else {
        config->monitoring.status_update_ms = DEFAULT_STATUS_UPDATE_MS;
        config->monitoring.resource_check_ms = DEFAULT_RESOURCE_CHECK_MS;
        config->monitoring.metrics_update_ms = DEFAULT_METRICS_UPDATE_MS;
        config->monitoring.memory_warning_percent = DEFAULT_MEMORY_WARNING_PERCENT;
        config->monitoring.disk_warning_percent = DEFAULT_DISK_WARNING_PERCENT;
        config->monitoring.load_warning = DEFAULT_LOAD_WARNING;
        
        log_config_section_header("SystemMonitoring");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_WARN);
    }

    // Print Queue Configuration
    json_t* print_queue = json_object_get(root, "PrintQueue");
    if (json_is_object(print_queue)) {
        log_config_section_header("PrintQueue");
        
        json_t* enabled = json_object_get(print_queue, "Enabled");
        config->print_queue.enabled = get_config_bool(enabled, 1);
        log_config_section_item("Enabled", "%s", LOG_LEVEL_INFO, 
                 config->print_queue.enabled ? "true" : "false");

        json_t* queue_settings = json_object_get(print_queue, "QueueSettings");
        if (json_is_object(queue_settings)) {
            log_config_section_item("QueueSettings", "Configured", LOG_LEVEL_INFO);
            
            json_t* val;
            val = json_object_get(queue_settings, "DefaultPriority");
            config->print_queue.priorities.default_priority = get_config_int(val, 1);
            log_config_section_item("DefaultPriority", "%d", LOG_LEVEL_INFO, config->print_queue.priorities.default_priority);
            
            val = json_object_get(queue_settings, "EmergencyPriority");
            config->print_queue.priorities.emergency_priority = get_config_int(val, 0);
            log_config_section_item("EmergencyPriority", "%d", LOG_LEVEL_INFO, config->print_queue.priorities.emergency_priority);
            
            val = json_object_get(queue_settings, "MaintenancePriority");
            config->print_queue.priorities.maintenance_priority = get_config_int(val, 2);
            log_config_section_item("MaintenancePriority", "%d", LOG_LEVEL_INFO, config->print_queue.priorities.maintenance_priority);
            
            val = json_object_get(queue_settings, "SystemPriority");
            config->print_queue.priorities.system_priority = get_config_int(val, 3);
            log_config_section_item("SystemPriority", "%d", LOG_LEVEL_INFO, config->print_queue.priorities.system_priority);
        }

        json_t* timeouts = json_object_get(print_queue, "Timeouts");
        if (json_is_object(timeouts)) {
            log_config_section_item("Timeouts", "Configured", LOG_LEVEL_INFO);
            
            json_t* val;
            val = json_object_get(timeouts, "ShutdownWaitMs");
            config->print_queue.timeouts.shutdown_wait_ms = get_config_size(val, DEFAULT_SHUTDOWN_WAIT_MS);
            log_config_section_item("ShutdownWaitMs", "%zu ms (%d seconds)", LOG_LEVEL_INFO, 
                   config->print_queue.timeouts.shutdown_wait_ms, 
                   (int)(config->print_queue.timeouts.shutdown_wait_ms / 1000));
            
            val = json_object_get(timeouts, "JobProcessingTimeoutMs");
            config->print_queue.timeouts.job_processing_timeout_ms = get_config_size(val, DEFAULT_JOB_PROCESSING_TIMEOUT_MS);
            log_config_section_item("JobProcessingTimeoutMs", "%zu ms", LOG_LEVEL_INFO, config->print_queue.timeouts.job_processing_timeout_ms);
        }

        json_t* buffers = json_object_get(print_queue, "Buffers");
        if (json_is_object(buffers)) {
            log_config_section_item("Buffers", "Configured", LOG_LEVEL_INFO);
            
            json_t* val;
            val = json_object_get(buffers, "JobMessageSize");
            config->print_queue.buffers.job_message_size = get_config_size(val, 256);
            log_config_section_item("JobMessageSize", "%zu bytes", LOG_LEVEL_INFO, config->print_queue.buffers.job_message_size);
            
            val = json_object_get(buffers, "StatusMessageSize");
            config->print_queue.buffers.status_message_size = get_config_size(val, 256);
            log_config_section_item("StatusMessageSize", "%zu bytes", LOG_LEVEL_INFO, config->print_queue.buffers.status_message_size);
        }
    } else {
        config->print_queue.enabled = 1;
        config->print_queue.priorities.default_priority = 1;
        config->print_queue.priorities.emergency_priority = 0;
        config->print_queue.priorities.maintenance_priority = 2;
        config->print_queue.priorities.system_priority = 3;
        config->print_queue.timeouts.shutdown_wait_ms = DEFAULT_SHUTDOWN_WAIT_MS;
        config->print_queue.timeouts.job_processing_timeout_ms = DEFAULT_JOB_PROCESSING_TIMEOUT_MS;
        config->print_queue.buffers.job_message_size = 256;
        config->print_queue.buffers.status_message_size = 256;
        
        log_config_section_header("PrintQueue");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_WARN);
    }

    // API Configuration
    json_t* api_config = json_object_get(root, "API");
    if (json_is_object(api_config)) {
        log_config_section_header("API");
        
        json_t* jwt_secret = json_object_get(api_config, "JWTSecret");
        config->api.jwt_secret = get_config_string(jwt_secret, "hydrogen_api_secret_change_me");
        log_config_section_item("JWTSecret", "%s", LOG_LEVEL_INFO, 
            strcmp(config->api.jwt_secret, "hydrogen_api_secret_change_me") == 0 ? "(default)" : "configured");
    } else {
        config->api.jwt_secret = strdup("hydrogen_api_secret_change_me");
        log_config_section_header("API");
        log_config_section_item("Status", "Section missing, using defaults", LOG_LEVEL_WARN);
    }

    json_decref(root);
    
    return config;
}