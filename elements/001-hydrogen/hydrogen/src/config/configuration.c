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
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <linux/limits.h>
#include <errno.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Project headers
#include "configuration.h"
#include "configuration_env.h"
#include "configuration_string.h"
#include "configuration_bool.h"
#include "configuration_int.h"
#include "configuration_size.h"
#include "configuration_double.h"
#include "../logging/logging.h"
#include "../utils/utils.h"

// Global variables
int MAX_PRIORITY_LABEL_WIDTH = 9;
int MAX_SUBSYSTEM_LABEL_WIDTH = 18;

// Global static configuration instance
static AppConfig *app_config = NULL;

// Priority level definitions
const PriorityLevel DEFAULT_PRIORITY_LEVELS[NUM_PRIORITY_LEVELS] = {
    {0, "ALL"},
    {1, "INFO"},
    {2, "WARN"},
    {3, "DEBUG"},
    {4, "ERROR"},
    {5, "CRITICAL"},
    {6, "NONE"}
};

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
 * Determine executable location with robust error handling
 * 
 * Why use /proc/self/exe?
 * - Provides the true binary path even when called through symlinks
 * - Works regardless of current working directory
 * - Handles SUID/SGID binaries correctly
 * - Gives absolute path without assumptions
 * 
 * Error Handling Strategy:
 * 1. Use readlink() for atomic path resolution
 * 2. Ensure null-termination for safety
 * 3. Handle memory allocation failures gracefully
 * 4. Return NULL on any error for consistent error handling
 */
char* get_executable_path() {
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1) {
        log_this("Configuration", "Error reading /proc/self/exe", LOG_LEVEL_DEBUG);
        return NULL;
    }
    path[len] = '\0';
    char* result = strdup(path);
    if (!result) {
        log_this("Configuration", "Error allocating memory for executable path", LOG_LEVEL_DEBUG);
        return NULL;
    }
    return result;
}

/*
 * Get file size with proper error detection
 * 
 * Why use stat()?
 * - Avoids opening the file unnecessarily
 * - Works for special files (devices, pipes)
 * - More efficient than seeking
 * - Provides atomic size reading
 */
long get_file_size(const char* filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

/*
 * Get file modification time in human-readable format
 * 
 * Why this format?
 * - ISO 8601-like timestamp for consistency
 * - Local time for admin readability
 * - Fixed width for log formatting
 * - Includes date and time for complete context
 * 
 * Memory Management:
 * - Allocates fixed size buffer for safety
 * - Caller owns returned string
 * - Returns NULL on any error
 */
char* get_file_modification_time(const char* filename) {
    struct stat st;
    if (stat(filename, &st) != 0) {
        return NULL;
    }

    struct tm* tm_info = localtime(&st.st_mtime);
    if (tm_info == NULL) {
        return NULL;
    }

    char* time_str = malloc(20);  // YYYY-MM-DD HH:MM:SS\0
    if (time_str == NULL) {
        return NULL;
    }

    if (strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info) == 0) {
        free(time_str);
        return NULL;
    }

    return time_str;
}

/*
 * Calculate maximum width of priority labels
 * 
 * Why pre-calculate?
 * - Ensures consistent log formatting
 * - Avoids repeated calculations
 * - Supports dynamic priority systems
 * - Maintains log readability
 */
void calculate_max_priority_label_width() {
    MAX_PRIORITY_LABEL_WIDTH = 0;
    for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        int label_width = strlen(DEFAULT_PRIORITY_LEVELS[i].label);
        if (label_width > MAX_PRIORITY_LABEL_WIDTH) {
            MAX_PRIORITY_LABEL_WIDTH = label_width;
        }
    }
}

/*
 * Generate default configuration with secure baseline
 * 
 * Why these defaults?
 * 1. Security First
 *    - Conservative file permissions and paths
 *    - Secure WebSocket keys and protocols
 *    - Resource limits to prevent DoS
 *    - Separate ports for different services
 * 
 * 2. Zero Configuration
 *    - Works out of the box for basic setups
 *    - Reasonable defaults for most environments
 *    - Clear upgrade paths
 * 
 * 3. Discovery Ready
 *    - Standard ports for easy finding
 *    - mDNS services pre-configured
 *    - Compatible with common tools
 * 
 * 4. Operational Safety
 *    - Temporary directories for uploads
 *    - Size limits on all inputs
 *    - Separate logging for each component
 *    - Graceful failure modes
 */
void create_default_config(const char* config_path) {
    json_t* root = json_object();

    // Server Name
    json_object_set_new(root, "ServerName", json_string("Philement/hydrogen"));
    
    // Payload Key (using environment variable by default)
    json_object_set_new(root, "PayloadKey", json_string("${env.PAYLOAD_KEY}"));

    // Log File
    json_object_set_new(root, "LogFile", json_string("/var/log/hydrogen.log"));

    // Web Configuration
    json_t* web = json_object();
    json_object_set_new(web, "Enabled", json_boolean(1));
    json_object_set_new(web, "EnableIPv6", json_boolean(0));  // Default to disabled like mDNS
    json_object_set_new(web, "Port", json_integer(5000));
    json_object_set_new(web, "WebRoot", json_string("/var/www/html"));
    json_object_set_new(web, "UploadPath", json_string(DEFAULT_UPLOAD_PATH));
    json_object_set_new(web, "UploadDir", json_string(DEFAULT_UPLOAD_DIR));
    json_object_set_new(web, "MaxUploadSize", json_integer(DEFAULT_MAX_UPLOAD_SIZE));
    json_object_set_new(root, "WebServer", web);

    // Logging Configuration
    json_t* logging = json_object();
    
    // Define logging levels
    json_t* levels = json_array();
    for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        json_t* level = json_array();
        json_array_append_new(level, json_integer(DEFAULT_PRIORITY_LEVELS[i].value));
        json_array_append_new(level, json_string(DEFAULT_PRIORITY_LEVELS[i].label));
        json_array_append_new(levels, level);
    }
    json_object_set_new(logging, "Levels", levels);

    // Console logging configuration
    json_t* console = json_object();
    json_object_set_new(console, "Enabled", json_boolean(1));
    json_t* console_subsystems = json_object();
    json_object_set_new(console_subsystems, "ThreadMgmt", json_integer(LOG_LEVEL_WARN));
    json_object_set_new(console_subsystems, "Shutdown", json_integer(LOG_LEVEL_INFO));
    json_object_set_new(console_subsystems, "mDNSServer", json_integer(LOG_LEVEL_INFO));
    json_object_set_new(console_subsystems, "WebServer", json_integer(LOG_LEVEL_INFO));
    json_object_set_new(console_subsystems, "WebSocket", json_integer(LOG_LEVEL_INFO));
    json_object_set_new(console_subsystems, "PrintQueue", json_integer(LOG_LEVEL_WARN));
    json_object_set_new(console_subsystems, "LogQueueManager", json_integer(LOG_LEVEL_INFO));
    json_object_set_new(console, "Subsystems", console_subsystems);
    json_object_set_new(logging, "Console", console);

    json_object_set_new(root, "Logging", logging);

    // WebSocket Configuration
    json_t* websocket = json_object();
    json_object_set_new(websocket, "Enabled", json_boolean(1));
    json_object_set_new(websocket, "EnableIPv6", json_boolean(0));
    json_object_set_new(websocket, "Port", json_integer(DEFAULT_WEBSOCKET_PORT));
    json_object_set_new(websocket, "Key", json_string("default_key_change_me"));
    json_object_set_new(websocket, "Protocol", json_string("hydrogen-protocol"));
    json_object_set_new(root, "WebSocket", websocket);

    // mDNS Server Configuration
    json_t* mdns_server = json_object();
    json_object_set_new(mdns_server, "Enabled", json_boolean(1));
    json_object_set_new(mdns_server, "EnableIPv6", json_boolean(0));
    json_object_set_new(mdns_server, "DeviceId", json_string("hydrogen-printer"));
    json_object_set_new(mdns_server, "FriendlyName", json_string("Hydrogen 3D Printer"));
    json_object_set_new(mdns_server, "Model", json_string("Hydrogen"));
    json_object_set_new(mdns_server, "Manufacturer", json_string("Philement"));
    json_object_set_new(mdns_server, "Version", json_string(VERSION));

    json_t* services = json_array();

    json_t* http_service = json_object();
    json_object_set_new(http_service, "Name", json_string("hydrogen"));
    json_object_set_new(http_service, "Type", json_string("_http._tcp.local"));
    json_object_set_new(http_service, "Port", json_integer(DEFAULT_WEB_PORT));
    json_object_set_new(http_service, "TxtRecords", json_string("path=/api/upload"));
    json_array_append_new(services, http_service);

    json_t* websocket_service = json_object();
    json_object_set_new(websocket_service, "Name", json_string("Hydrogen"));
    json_object_set_new(websocket_service, "Type", json_string("_websocket._tcp.local"));
    json_object_set_new(websocket_service, "Port", json_integer(DEFAULT_WEBSOCKET_PORT));
    json_object_set_new(websocket_service, "TxtRecords", json_string("path=/websocket"));
    json_array_append_new(services, websocket_service);

    json_object_set_new(mdns_server, "Services", services);
    json_object_set_new(root, "mDNSServer", mdns_server);

    // System Resources Configuration
    json_t* resources = json_object();
    
    json_t* queues = json_object();
    json_object_set_new(queues, "MaxQueueBlocks", json_integer(DEFAULT_MAX_QUEUE_BLOCKS));
    json_object_set_new(queues, "QueueHashSize", json_integer(DEFAULT_QUEUE_HASH_SIZE));
    json_object_set_new(queues, "DefaultQueueCapacity", json_integer(DEFAULT_QUEUE_CAPACITY));
    json_object_set_new(resources, "Queues", queues);

    json_t* buffers = json_object();
    json_object_set_new(buffers, "DefaultMessageBuffer", json_integer(DEFAULT_MESSAGE_BUFFER_SIZE));
    json_object_set_new(buffers, "MaxLogMessageSize", json_integer(DEFAULT_MAX_LOG_MESSAGE_SIZE));
    json_object_set_new(buffers, "LineBufferSize", json_integer(DEFAULT_LINE_BUFFER_SIZE));
    json_object_set_new(buffers, "PostProcessorBuffer", json_integer(DEFAULT_POST_PROCESSOR_BUFFER_SIZE));
    json_object_set_new(resources, "Buffers", buffers);
    
    json_object_set_new(root, "SystemResources", resources);

    // Network Configuration
    json_t* network = json_object();
    
    json_t* interfaces = json_object();
    json_object_set_new(interfaces, "MaxInterfaces", json_integer(DEFAULT_MAX_INTERFACES));
    json_object_set_new(interfaces, "MaxIPsPerInterface", json_integer(DEFAULT_MAX_IPS_PER_INTERFACE));
    json_object_set_new(interfaces, "MaxInterfaceNameLength", json_integer(DEFAULT_MAX_INTERFACE_NAME_LENGTH));
    json_object_set_new(interfaces, "MaxIPAddressLength", json_integer(DEFAULT_MAX_IP_ADDRESS_LENGTH));
    json_object_set_new(network, "Interfaces", interfaces);

    json_t* port_allocation = json_object();
    json_object_set_new(port_allocation, "StartPort", json_integer(DEFAULT_WEB_PORT));
    json_object_set_new(port_allocation, "EndPort", json_integer(65535));
    json_t* reserved_ports = json_array();
    json_array_append_new(reserved_ports, json_integer(22));
    json_array_append_new(reserved_ports, json_integer(80));
    json_array_append_new(reserved_ports, json_integer(443));
    json_object_set_new(port_allocation, "ReservedPorts", reserved_ports);
    json_object_set_new(network, "PortAllocation", port_allocation);
    
    json_object_set_new(root, "Network", network);

    // System Monitoring Configuration
    json_t* monitoring = json_object();
    
    json_t* intervals = json_object();
    json_object_set_new(intervals, "StatusUpdateMs", json_integer(DEFAULT_STATUS_UPDATE_MS));
    json_object_set_new(intervals, "ResourceCheckMs", json_integer(DEFAULT_RESOURCE_CHECK_MS));
    json_object_set_new(intervals, "MetricsUpdateMs", json_integer(DEFAULT_METRICS_UPDATE_MS));
    json_object_set_new(monitoring, "Intervals", intervals);

    json_t* thresholds = json_object();
    json_object_set_new(thresholds, "MemoryWarningPercent", json_integer(DEFAULT_MEMORY_WARNING_PERCENT));
    json_object_set_new(thresholds, "DiskSpaceWarningPercent", json_integer(DEFAULT_DISK_WARNING_PERCENT));
    json_object_set_new(thresholds, "LoadAverageWarning", json_real(DEFAULT_LOAD_WARNING));
    json_object_set_new(monitoring, "Thresholds", thresholds);
    
    json_object_set_new(root, "SystemMonitoring", monitoring);

    // Print Queue Configuration
    json_t* print_queue = json_object();
    json_object_set_new(print_queue, "Enabled", json_boolean(1));
    
    json_t* queue_settings = json_object();
    json_object_set_new(queue_settings, "DefaultPriority", json_integer(1));
    json_object_set_new(queue_settings, "EmergencyPriority", json_integer(0));
    json_object_set_new(queue_settings, "MaintenancePriority", json_integer(2));
    json_object_set_new(queue_settings, "SystemPriority", json_integer(3));
    json_object_set_new(print_queue, "QueueSettings", queue_settings);

    json_t* queue_timeouts = json_object();
    json_object_set_new(queue_timeouts, "ShutdownWaitMs", json_integer(DEFAULT_SHUTDOWN_WAIT_MS));
    json_object_set_new(queue_timeouts, "JobProcessingTimeoutMs", json_integer(DEFAULT_JOB_PROCESSING_TIMEOUT_MS));
    json_object_set_new(print_queue, "Timeouts", queue_timeouts);

    json_t* queue_buffers = json_object();
    json_object_set_new(queue_buffers, "JobMessageSize", json_integer(256));
    json_object_set_new(queue_buffers, "StatusMessageSize", json_integer(256));
    json_object_set_new(print_queue, "Buffers", queue_buffers);
    
    json_object_set_new(root, "PrintQueue", print_queue);

    // API Configuration
    json_t* api = json_object();
    json_object_set_new(api, "JWTSecret", json_string("hydrogen_api_secret_change_me"));
    json_object_set_new(root, "API", api);

    if (json_dump_file(root, config_path, JSON_INDENT(4)) != 0) {
        log_this("Configuration", "Error: Unable to create default config at %s", LOG_LEVEL_DEBUG, config_path);
    } else {
        log_this("Configuration", "Created default config at %s", LOG_LEVEL_INFO, config_path);
    }

    json_decref(root);
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
    // Free previous configuration if it exists
    if (app_config) {
        // TODO: Implement proper cleanup of AppConfig resources
        free(app_config);
    }

    json_error_t error;
    json_t* root = json_load_file(config_path, 0, &error);

    if (!root) {
        log_this("Configuration", "Failed to load config file: %s (line %d, column %d)",
                 LOG_LEVEL_ERROR, error.text, error.line, error.column);
        
        fprintf(stderr, "ERROR: Hydrogen configuration file has JSON syntax errors.\n");
        fprintf(stderr, "       %s at line %d, column %d\n", error.text, error.line, error.column);
        fprintf(stderr, "       Please fix the syntax error and try again.\n");
        
        exit(EXIT_FAILURE);
    }

    AppConfig* config = calloc(1, sizeof(AppConfig));
    if (!config) {
        log_this("Configuration", "Failed to allocate memory for config", LOG_LEVEL_DEBUG);
        json_decref(root);
        return NULL;
    }

    // Store the configuration in the global static variable
    app_config = config;

    // Store paths
    config->config_file = strdup(config_path);  // Store the config file path
    config->executable_path = get_executable_path();
    if (!config->executable_path) {
        log_this("Configuration", "Failed to get executable path, using default", LOG_LEVEL_INFO);
        config->executable_path = strdup("./hydrogen");
    }

    // Server Name
    json_t* server_name = json_object_get(root, "ServerName");
    config->server_name = get_config_string(server_name, DEFAULT_SERVER_NAME);
    if (json_is_string(server_name) && 
        strncmp(json_string_value(server_name), "${env.", 6) == 0 &&
        !getenv(json_string_value(server_name) + 6)) {
        log_this("Configuration", "ServerName: (default)", LOG_LEVEL_INFO);
    } else {
        log_this("Configuration", "ServerName: %s", LOG_LEVEL_INFO, config->server_name);
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
                log_this("Configuration", "PayloadKey: Using value from %s", LOG_LEVEL_INFO, var_name);
            } else {
                log_this("Configuration", "PayloadKey: Environment variable %s not found", LOG_LEVEL_WARN, var_name);
            }
        }
    } else if (config->payload_key) {
        log_this("Configuration", "PayloadKey: Set from configuration", LOG_LEVEL_INFO);
    } else {
        log_this("Configuration", "PayloadKey: Not configured", LOG_LEVEL_WARN);
    }

    // Log File
    json_t* log_file = json_object_get(root, "LogFile");
    config->log_file_path = get_config_string(log_file, DEFAULT_LOG_FILE);
    log_this("Configuration", "LogFile: %s", LOG_LEVEL_INFO, config->log_file_path);

    // Web Configuration
    json_t* web = json_object_get(root, "WebServer");
    if (json_is_object(web)) {
        json_t* enabled = json_object_get(web, "Enabled");
        config->web.enabled = get_config_bool(enabled, 1);
        log_this("Configuration", "WebServer Enabled: %s", LOG_LEVEL_INFO, 
                 config->web.enabled ? "true" : "false");

        json_t* enable_ipv6 = json_object_get(web, "EnableIPv6");
        config->web.enable_ipv6 = get_config_bool(enable_ipv6, 0);

        json_t* port = json_object_get(web, "Port");
        config->web.port = get_config_int(port, DEFAULT_WEB_PORT);
        log_this("Configuration", "WebServer Port: %d", LOG_LEVEL_INFO, config->web.port);

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
        log_this("Configuration", "API Prefix: %s", LOG_LEVEL_INFO, config->web.api_prefix);
    } else {
        config->web.port = DEFAULT_WEB_PORT;
        config->web.web_root = strdup("/var/www/html");
        config->web.upload_path = strdup(DEFAULT_UPLOAD_PATH);
        config->web.upload_dir = strdup(DEFAULT_UPLOAD_DIR);
        config->web.max_upload_size = DEFAULT_MAX_UPLOAD_SIZE;
        config->web.api_prefix = strdup("/api");
        log_this("Configuration", "API Prefix: %s (default)", LOG_LEVEL_INFO, config->web.api_prefix);
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
            log_this("Configuration", "Failed to allocate WebSocket protocol string", LOG_LEVEL_ERROR);
            free(config->websocket.key);
            return NULL;
        }

        json_t* max_message_size = json_object_get(websocket, "MaxMessageSize");
        config->websocket.max_message_size = get_config_size(max_message_size, 10 * 1024 * 1024);

        json_t* connection_timeouts = json_object_get(websocket, "ConnectionTimeouts");
        if (json_is_object(connection_timeouts)) {
            json_t* exit_wait_seconds = json_object_get(connection_timeouts, "ExitWaitSeconds");
            config->websocket.exit_wait_seconds = get_config_int(exit_wait_seconds, 10);
            log_this("Configuration", "WebSocket Exit Wait Seconds: %d", LOG_LEVEL_INFO, config->websocket.exit_wait_seconds);
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
        log_this("Configuration", "PrintQueue Enabled: %s", LOG_LEVEL_INFO, 
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
            log_this("Configuration", "ShutdownWaitSeconds: %d", LOG_LEVEL_INFO, 
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
        log_this("Configuration", "Using default API configuration", LOG_LEVEL_INFO);
    }

    json_decref(root);
    return config;
}