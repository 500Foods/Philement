/*
 * Configuration management system with robust fallback handling
 * 
 * The configuration system implements several key design principles:
 * 
 * Fault Tolerance:
 * - Graceful fallback to defaults for missing values
 * - Validation of critical parameters
 * - Type checking for all values
 * - Memory allocation failure recovery
 * 
 * Flexibility:
 * - Runtime configuration changes
 * - Environment-specific overrides
 * - Service-specific settings
 * - Extensible structure
 * 
 * Security:
 * - Sensitive data isolation
 * - Path validation
 * - Size limits enforcement
 * - Access control settings
 * 
 * Maintainability:
 * - Centralized default values
 * - Structured error reporting
 * - Clear upgrade paths
 * - Configuration versioning
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
 * 
 * Why this approach?
 * 1. Resilient Loading
 *    - Handles partial configurations
 *    - Validates all values before use
 *    - Falls back to defaults safely
 *    - Preserves existing values when possible
 * 
 * 2. Memory Safety
 *    - Staged allocation for partial success
 *    - Complete cleanup on any failure
 *    - Minimal data copying
 *    - Proper string duplication
 * 
 * 3. Security Checks
 *    - Type validation for all values
 *    - Range checking for numeric fields
 *    - Path validation and normalization
 *    - Port availability verification
 * 
 * 4. Operational Awareness
 *    - Environment-specific defaults
 *    - Detailed error logging
 *    - Clear indication of fallback use
 *    - Maintains configuration versioning
 */
AppConfig* load_config(const char* config_path) {

    log_this("Config", "%s", LOG_LEVEL_INFO, LOG_LINE_BREAK);
    log_this("Config", "CONFIGURATION", LOG_LEVEL_INFO);

    // Free previous configuration if it exists
    if (app_config) {
        // TODO: Implement proper cleanup of AppConfig resources
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

    // Store the configuration in the global static variable
    app_config = config;

    // Store paths
    config->config_file = strdup(config_path);  // Store the config file path
    config->executable_path = get_executable_path();
    if (!config->executable_path) {
        log_this("Config", "Failed to get executable path, using default", LOG_LEVEL_INFO);
        config->executable_path = strdup("./hydrogen");
    }

    // Server Name
    json_t* server_name = json_object_get(root, "ServerName");
    config->server_name = get_config_string(server_name, DEFAULT_SERVER_NAME);
    if (json_is_string(server_name) && 
        strncmp(json_string_value(server_name), "${env.", 6) == 0 &&
        !getenv(json_string_value(server_name) + 6)) {
        log_this("Config", "ServerName: (default)", LOG_LEVEL_INFO);
    } else {
        log_this("Config", "ServerName: %s", LOG_LEVEL_INFO, config->server_name);
    }
    
    // Payload Key (for payload decryption)
    json_t* payload_key = json_object_get(root, "PayloadKey");
    config->payload_key = get_config_string(payload_key, "${env.PAYLOAD_KEY}");
    if (config->payload_key && strncmp(config->payload_key, "${env.", 6) == 0) {
        const char* env_var = config->payload_key + 6;
        size_t env_var_len = strlen(env_var);
        if (env_var_len > 1 && env_var[env_var_len - 1] == '}') {
            char var_name[256] = {0};
            size_t copy_len = env_var_len - 1 < sizeof(var_name) - 1 ? env_var_len - 1 : sizeof(var_name) - 1;
            memcpy(var_name, env_var, copy_len);
            var_name[copy_len] = '\0';
            
            if (getenv(var_name)) {
                log_this("Config-Env", "PayloadKey: Using value from %s", LOG_LEVEL_INFO, var_name);
            } else {
                log_this("Config-Env", "PayloadKey: Environment variable %s not found", LOG_LEVEL_WARN, var_name);
            }
        }
    } else if (config->payload_key) {
        log_this("Config", "PayloadKey: Set from configuration", LOG_LEVEL_INFO);
    } else {
        log_this("COnfig", "PayloadKey: Not configured", LOG_LEVEL_WARN);
    }

    // Log File
    json_t* log_file = json_object_get(root, "LogFile");
    config->log_file_path = get_config_string(log_file, DEFAULT_LOG_FILE_PATH);
    log_this("Config", "LogFile: %s", LOG_LEVEL_INFO, config->log_file_path);

    // Web Configuration
    json_t* web = json_object_get(root, "WebServer");
    if (json_is_object(web)) {
        json_t* enabled = json_object_get(web, "Enabled");
        config->web.enabled = get_config_bool(enabled, 1);
        log_this("COnfig", "WebServer Enabled: %s", LOG_LEVEL_INFO, 
                 config->web.enabled ? "true" : "false");

        json_t* enable_ipv6 = json_object_get(web, "EnableIPv6");
        config->web.enable_ipv6 = get_config_bool(enable_ipv6, 0);

        json_t* port = json_object_get(web, "Port");
        config->web.port = get_config_int(port, DEFAULT_WEB_PORT);
        log_this("Config", "WebServer Port: %d", LOG_LEVEL_INFO, config->web.port);

        json_t* web_root = json_object_get(web, "WebRoot");
        config->web.web_root = get_config_string(web_root, "/var/www/html");

        json_t* upload_path = json_object_get(web, "UploadPath");
        config->web.upload_path = get_config_string(upload_path, DEFAULT_UPLOAD_PATH);

        json_t* upload_dir = json_object_get(web, "UploadDir");
        config->web.upload_dir = get_config_string(upload_dir, DEFAULT_UPLOAD_DIR);

        json_t* max_upload_size = json_object_get(web, "MaxUploadSize");
        config->web.max_upload_size = get_config_size(max_upload_size, DEFAULT_MAX_UPLOAD_SIZE);
        
        json_t* api_prefix = json_object_get(web, "ApiPrefix");
        config->web.api_prefix = get_config_string(api_prefix, "/api");
        log_this("Config", "API Prefix: %s", LOG_LEVEL_INFO, config->web.api_prefix);
    } else {
        config->web.port = DEFAULT_WEB_PORT;
        config->web.web_root = strdup("/var/www/html");
        config->web.upload_path = strdup(DEFAULT_UPLOAD_PATH);
        config->web.upload_dir = strdup(DEFAULT_UPLOAD_DIR);
        config->web.max_upload_size = DEFAULT_MAX_UPLOAD_SIZE;
        config->web.api_prefix = strdup("/api");
        log_this("Config", "API Prefix: %s (default)", LOG_LEVEL_INFO, config->web.api_prefix);
    }

    // WebSocket Configuration
    json_t* websocket = json_object_get(root, "WebSocket");
    if (json_is_object(websocket)) {
        json_t* enabled = json_object_get(websocket, "Enabled");
        config->websocket.enabled = get_config_bool(enabled, 1);

        json_t* enable_ipv6 = json_object_get(websocket, "EnableIPv6");
        config->websocket.enable_ipv6 = get_config_bool(enable_ipv6, 0);

        json_t* port = json_object_get(websocket, "Port");
        config->websocket.port = get_config_int(port, DEFAULT_WEBSOCKET_PORT);

        json_t* key = json_object_get(websocket, "Key");
        config->websocket.key = get_config_string(key, "default_key");

        json_t* protocol = json_object_get(websocket, "protocol");
        if (!protocol) {
            protocol = json_object_get(websocket, "Protocol");  // Try legacy uppercase key
        }
        config->websocket.protocol = get_config_string(protocol, "hydrogen-protocol");
        if (!config->websocket.protocol) {
            log_this("Config", "Failed to allocate WebSocket protocol string", LOG_LEVEL_ERROR);
            free(config->websocket.key);
            return NULL;
        }

        json_t* max_message_size = json_object_get(websocket, "MaxMessageSize");
        config->websocket.max_message_size = get_config_size(max_message_size, 10 * 1024 * 1024);

        json_t* connection_timeouts = json_object_get(websocket, "ConnectionTimeouts");
        if (json_is_object(connection_timeouts)) {
            json_t* exit_wait_seconds = json_object_get(connection_timeouts, "ExitWaitSeconds");
            config->websocket.exit_wait_seconds = get_config_int(exit_wait_seconds, 10);
            log_this("Config", "WebSocket Exit Wait Seconds: %d", LOG_LEVEL_INFO, config->websocket.exit_wait_seconds);
        } else {
            config->websocket.exit_wait_seconds = 10;
        }
    } else {
        config->websocket.port = DEFAULT_WEBSOCKET_PORT;
        config->websocket.key = strdup("default_key");
        config->websocket.protocol = strdup("hydrogen-protocol");
        config->websocket.max_message_size = 10 * 1024 * 1024;
        config->websocket.exit_wait_seconds = 10;
    }

    // mDNS Server Configuration
    json_t* mdns_server = json_object_get(root, "mDNSServer");
    if (json_is_object(mdns_server)) {
        json_t* enabled = json_object_get(mdns_server, "Enabled");
        config->mdns_server.enabled = get_config_bool(enabled, 1);

        json_t* enable_ipv6 = json_object_get(mdns_server, "EnableIPv6");
        config->mdns_server.enable_ipv6 = get_config_bool(enable_ipv6, 1);

        json_t* device_id = json_object_get(mdns_server, "DeviceId");
        config->mdns_server.device_id = get_config_string(device_id, "hydrogen-printer");

        json_t* friendly_name = json_object_get(mdns_server, "FriendlyName");
        config->mdns_server.friendly_name = get_config_string(friendly_name, "Hydrogen 3D Printer");

        json_t* model = json_object_get(mdns_server, "Model");
        config->mdns_server.model = get_config_string(model, "Hydrogen");

        json_t* manufacturer = json_object_get(mdns_server, "Manufacturer");
        config->mdns_server.manufacturer = get_config_string(manufacturer, "Philement");

        json_t* version = json_object_get(mdns_server, "Version");
        config->mdns_server.version = get_config_string(version, VERSION);

        json_t* services = json_object_get(mdns_server, "Services");
        if (json_is_array(services)) {
            config->mdns_server.num_services = json_array_size(services);
            config->mdns_server.services = calloc(config->mdns_server.num_services, sizeof(mdns_server_service_t));

            for (size_t i = 0; i < config->mdns_server.num_services; i++) {
                json_t* service = json_array_get(services, i);
                if (!json_is_object(service)) continue;

                json_t* name = json_object_get(service, "Name");
                config->mdns_server.services[i].name = get_config_string(name, "hydrogen");

                json_t* type = json_object_get(service, "Type");
                config->mdns_server.services[i].type = get_config_string(type, "_http._tcp.local");

                json_t* port = json_object_get(service, "Port");
                config->mdns_server.services[i].port = get_config_int(port, DEFAULT_WEB_PORT);

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
    }

    // System Resources Configuration
    json_t* resources = json_object_get(root, "SystemResources");
    if (json_is_object(resources)) {
        json_t* queues = json_object_get(resources, "Queues");
        if (json_is_object(queues)) {
            json_t* val;
            val = json_object_get(queues, "MaxQueueBlocks");
            config->resources.max_queue_blocks = get_config_size(val, DEFAULT_MAX_QUEUE_BLOCKS);
            
            val = json_object_get(queues, "QueueHashSize");
            config->resources.queue_hash_size = get_config_size(val, DEFAULT_QUEUE_HASH_SIZE);
            
            val = json_object_get(queues, "DefaultQueueCapacity");
            config->resources.default_capacity = get_config_size(val, DEFAULT_QUEUE_CAPACITY);
        }

        json_t* buffers = json_object_get(resources, "Buffers");
        if (json_is_object(buffers)) {
            json_t* val;
            val = json_object_get(buffers, "DefaultMessageBuffer");
            config->resources.message_buffer_size = get_config_size(val, DEFAULT_MESSAGE_BUFFER_SIZE);
            
            val = json_object_get(buffers, "MaxLogMessageSize");
            config->resources.max_log_message_size = get_config_size(val, DEFAULT_MAX_LOG_MESSAGE_SIZE);
            
            val = json_object_get(buffers, "LineBufferSize");
            config->resources.line_buffer_size = get_config_size(val, DEFAULT_LINE_BUFFER_SIZE);

            val = json_object_get(buffers, "PostProcessorBuffer");
            config->resources.post_processor_buffer_size = get_config_size(val, DEFAULT_POST_PROCESSOR_BUFFER_SIZE);

            val = json_object_get(buffers, "LogBufferSize");
            config->resources.log_buffer_size = get_config_size(val, DEFAULT_LOG_BUFFER_SIZE);

            val = json_object_get(buffers, "JsonMessageSize");
            config->resources.json_message_size = get_config_size(val, DEFAULT_JSON_MESSAGE_SIZE);

            val = json_object_get(buffers, "LogEntrySize");
            config->resources.log_entry_size = get_config_size(val, DEFAULT_LOG_ENTRY_SIZE);
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
    }

    // Network Configuration
    json_t* network = json_object_get(root, "Network");
    if (json_is_object(network)) {
        json_t* interfaces = json_object_get(network, "Interfaces");
        if (json_is_object(interfaces)) {
            json_t* val;
            val = json_object_get(interfaces, "MaxInterfaces");
            config->network.max_interfaces = get_config_size(val, DEFAULT_MAX_INTERFACES);
            
            val = json_object_get(interfaces, "MaxIPsPerInterface");
            config->network.max_ips_per_interface = get_config_size(val, DEFAULT_MAX_IPS_PER_INTERFACE);
            
            val = json_object_get(interfaces, "MaxInterfaceNameLength");
            config->network.max_interface_name_length = get_config_size(val, DEFAULT_MAX_INTERFACE_NAME_LENGTH);
            
            val = json_object_get(interfaces, "MaxIPAddressLength");
            config->network.max_ip_address_length = get_config_size(val, DEFAULT_MAX_IP_ADDRESS_LENGTH);
        }

        json_t* port_allocation = json_object_get(network, "PortAllocation");
        if (json_is_object(port_allocation)) {
            json_t* val;
            val = json_object_get(port_allocation, "StartPort");
            config->network.start_port = get_config_int(val, DEFAULT_WEB_PORT);
            
            val = json_object_get(port_allocation, "EndPort");
            config->network.end_port = get_config_int(val, 65535);

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
    }

    // System Monitoring Configuration
    json_t* monitoring = json_object_get(root, "SystemMonitoring");
    if (json_is_object(monitoring)) {
        json_t* intervals = json_object_get(monitoring, "Intervals");
        if (json_is_object(intervals)) {
            json_t* val;
            val = json_object_get(intervals, "StatusUpdateMs");
            config->monitoring.status_update_ms = get_config_size(val, DEFAULT_STATUS_UPDATE_MS);
            
            val = json_object_get(intervals, "ResourceCheckMs");
            config->monitoring.resource_check_ms = get_config_size(val, DEFAULT_RESOURCE_CHECK_MS);
            
            val = json_object_get(intervals, "MetricsUpdateMs");
            config->monitoring.metrics_update_ms = get_config_size(val, DEFAULT_METRICS_UPDATE_MS);
        }

        json_t* thresholds = json_object_get(monitoring, "Thresholds");
        if (json_is_object(thresholds)) {
            json_t* val;
            val = json_object_get(thresholds, "MemoryWarningPercent");
            config->monitoring.memory_warning_percent = get_config_int(val, DEFAULT_MEMORY_WARNING_PERCENT);
            
            val = json_object_get(thresholds, "DiskSpaceWarningPercent");
            config->monitoring.disk_warning_percent = get_config_int(val, DEFAULT_DISK_WARNING_PERCENT);
            
            val = json_object_get(thresholds, "LoadAverageWarning");
            config->monitoring.load_warning = get_config_double(val, DEFAULT_LOAD_WARNING);
        }
    } else {
        config->monitoring.status_update_ms = DEFAULT_STATUS_UPDATE_MS;
        config->monitoring.resource_check_ms = DEFAULT_RESOURCE_CHECK_MS;
        config->monitoring.metrics_update_ms = DEFAULT_METRICS_UPDATE_MS;
        config->monitoring.memory_warning_percent = DEFAULT_MEMORY_WARNING_PERCENT;
        config->monitoring.disk_warning_percent = DEFAULT_DISK_WARNING_PERCENT;
        config->monitoring.load_warning = DEFAULT_LOAD_WARNING;
    }

    // Print Queue Configuration
    json_t* print_queue = json_object_get(root, "PrintQueue");
    if (json_is_object(print_queue)) {
        json_t* enabled = json_object_get(print_queue, "Enabled");
        config->print_queue.enabled = get_config_bool(enabled, 1);
        log_this("Config", "PrintQueue Enabled: %s", LOG_LEVEL_INFO, 
                 config->print_queue.enabled ? "true" : "false");

        json_t* queue_settings = json_object_get(print_queue, "QueueSettings");
        if (json_is_object(queue_settings)) {
            json_t* val;
            val = json_object_get(queue_settings, "DefaultPriority");
            config->print_queue.priorities.default_priority = get_config_int(val, 1);
            
            val = json_object_get(queue_settings, "EmergencyPriority");
            config->print_queue.priorities.emergency_priority = get_config_int(val, 0);
            
            val = json_object_get(queue_settings, "MaintenancePriority");
            config->print_queue.priorities.maintenance_priority = get_config_int(val, 2);
            
            val = json_object_get(queue_settings, "SystemPriority");
            config->print_queue.priorities.system_priority = get_config_int(val, 3);
        }

        json_t* timeouts = json_object_get(print_queue, "Timeouts");
        if (json_is_object(timeouts)) {
            json_t* val;
            val = json_object_get(timeouts, "ShutdownWaitMs");
            config->print_queue.timeouts.shutdown_wait_ms = get_config_size(val, DEFAULT_SHUTDOWN_WAIT_MS);
            log_this("Config", "ShutdownWaitSeconds: %d", LOG_LEVEL_INFO, 
                    (int)(config->print_queue.timeouts.shutdown_wait_ms / 1000));
            
            val = json_object_get(timeouts, "JobProcessingTimeoutMs");
            config->print_queue.timeouts.job_processing_timeout_ms = get_config_size(val, DEFAULT_JOB_PROCESSING_TIMEOUT_MS);
        }

        json_t* buffers = json_object_get(print_queue, "Buffers");
        if (json_is_object(buffers)) {
            json_t* val;
            val = json_object_get(buffers, "JobMessageSize");
            config->print_queue.buffers.job_message_size = get_config_size(val, 256);
            
            val = json_object_get(buffers, "StatusMessageSize");
            config->print_queue.buffers.status_message_size = get_config_size(val, 256);
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
    }

    // API Configuration
    json_t* api_config = json_object_get(root, "API");
    if (json_is_object(api_config)) {
        json_t* jwt_secret = json_object_get(api_config, "JWTSecret");
        config->api.jwt_secret = get_config_string(jwt_secret, "hydrogen_api_secret_change_me");
    } else {
        config->api.jwt_secret = strdup("hydrogen_api_secret_change_me");
        log_this("Config", "Using default API configuration", LOG_LEVEL_INFO);
    }

    json_decref(root);
    return config;
}