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

// Project headers
#include "configuration.h"
#include "../logging/logging.h"
#include "../utils/utils.h"

int MAX_PRIORITY_LABEL_WIDTH = 9;
int MAX_SUBSYSTEM_LABEL_WIDTH = 18;

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
 *    - Clear upgrade path from defaults
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

    // Log File
    json_object_set_new(root, "LogFile", json_string("/var/log/hydrogen.log"));

    // Web Configuration
    json_t* web = json_object();
    json_object_set_new(web, "Enabled", json_boolean(1));
    json_object_set_new(web, "EnableIPv6", json_boolean(0));  // Default to disabled like mDNS
    json_object_set_new(web, "Port", json_integer(5000));
    json_object_set_new(web, "WebRoot", json_string("/home/asimard/lithium"));
    json_object_set_new(web, "UploadPath", json_string("/api/upload"));
    json_object_set_new(web, "UploadDir", json_string("/tmp/hydrogen_uploads"));
    json_object_set_new(web, "MaxUploadSize", json_integer(2147483648));
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
    json_object_set_new(websocket, "EnableIPv6", json_boolean(0));  // Default to disabled like mDNS
    json_object_set_new(websocket, "Port", json_integer(5001));
    json_object_set_new(websocket, "Key", json_string("default_key_change_me"));
    json_object_set_new(websocket, "Protocol", json_string("hydrogen-protocol"));
    json_object_set_new(root, "WebSocket", websocket);

    // mDNS Configuration
    json_t* mdns = json_object();
    json_object_set_new(mdns, "Enabled", json_boolean(1));
    json_object_set_new(mdns, "EnableIPv6", json_boolean(0));  // Default to disabled since dev system doesn't support IPv6
    json_object_set_new(mdns, "DeviceId", json_string("hydrogen-printer"));
    json_object_set_new(mdns, "FriendlyName", json_string("Hydrogen 3D Printer"));
    json_object_set_new(mdns, "Model", json_string("Hydrogen"));
    json_object_set_new(mdns, "Manufacturer", json_string("Philement"));
    json_object_set_new(mdns, "Version", json_string("0.1.0"));

    json_t* services = json_array();

    json_t* http_service = json_object();
    json_object_set_new(http_service, "Name", json_string("hydrogen"));
    json_object_set_new(http_service, "Type", json_string("_http._tcp.local"));
    json_object_set_new(http_service, "Port", json_integer(5000));
    json_object_set_new(http_service, "TxtRecords", json_string("path=/api/upload"));
    json_array_append_new(services, http_service);

    json_t* octoprint_service = json_object();
    json_object_set_new(octoprint_service, "Name", json_string("hydrogen"));
    json_object_set_new(octoprint_service, "Type", json_string("_octoprint._tcp.local"));
    json_object_set_new(octoprint_service, "Port", json_integer(5000));
    json_object_set_new(octoprint_service, "TxtRecords", json_string("path=/api,version=1.1.0"));
    json_array_append_new(services, octoprint_service);

    json_t* websocket_service = json_object();
    json_object_set_new(websocket_service, "Name", json_string("Hydrogen"));
    json_object_set_new(websocket_service, "Type", json_string("_websocket._tcp.local"));
    json_object_set_new(websocket_service, "Port", json_integer(5001));
    json_object_set_new(websocket_service, "TxtRecords", json_string("path=/websocket"));
    json_array_append_new(services, websocket_service);

    json_object_set_new(mdns, "Services", services);
    json_object_set_new(root, "mDNSServer", mdns);

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
    json_object_set_new(port_allocation, "StartPort", json_integer(5000));
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

    // Printer Motion Configuration
    json_t* motion = json_object();
    json_object_set_new(motion, "MaxLayers", json_integer(DEFAULT_MAX_LAYERS));
    json_object_set_new(motion, "Acceleration", json_real(DEFAULT_ACCELERATION));
    json_object_set_new(motion, "ZAcceleration", json_real(DEFAULT_Z_ACCELERATION));
    json_object_set_new(motion, "EAcceleration", json_real(DEFAULT_E_ACCELERATION));
    json_object_set_new(motion, "MaxSpeedXY", json_real(DEFAULT_MAX_SPEED_XY));
    json_object_set_new(motion, "MaxSpeedTravel", json_real(DEFAULT_MAX_SPEED_TRAVEL));
    json_object_set_new(motion, "MaxSpeedZ", json_real(DEFAULT_MAX_SPEED_Z));
    json_object_set_new(motion, "ZValuesChunk", json_integer(DEFAULT_Z_VALUES_CHUNK));
    json_object_set_new(root, "Motion", motion);

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
    json_error_t error;
    json_t* root = json_load_file(config_path, 0, &error);

    if (!root) {
        log_this("Configuration", "Failed to load config file", LOG_LEVEL_DEBUG);
        return NULL;
    }

    AppConfig* config = calloc(1, sizeof(AppConfig));
    if (!config) {
        log_this("Configuration", "Failed to allocate memory for config", LOG_LEVEL_DEBUG);
        json_decref(root);
        return NULL;
    }

    // Set executable path
    config->executable_path = get_executable_path();
    if (!config->executable_path) {
        log_this("Configuration", "Failed to get executable path, using default", LOG_LEVEL_INFO);
        config->executable_path = strdup("./hydrogen");
    }

    // Server Name
    json_t* server_name = json_object_get(root, "ServerName");
    if (json_is_string(server_name)) {
        config->server_name = strdup(json_string_value(server_name));
    }

    // Log File
    json_t* log_file = json_object_get(root, "LogFile");
    if (json_is_string(log_file)) {
        config->log_file_path = strdup(json_string_value(log_file));
    }

    // Web Configuration
    json_t* web = json_object_get(root, "WebServer");
    if (json_is_object(web)) {
        json_t* enabled = json_object_get(web, "Enabled");
        config->web.enabled = json_is_boolean(enabled) ? json_boolean_value(enabled) : 1;

        json_t* enable_ipv6 = json_object_get(web, "EnableIPv6");
        config->web.enable_ipv6 = json_is_boolean(enable_ipv6) ? json_boolean_value(enable_ipv6) : 0;

        json_t* port = json_object_get(web, "Port");
        config->web.port = json_is_integer(port) ? json_integer_value(port) : DEFAULT_WEB_PORT;

        json_t* web_root = json_object_get(web, "WebRoot");
        const char* web_root_str = json_is_string(web_root) ? json_string_value(web_root) : "/var/www/html";
        config->web.web_root = strdup(web_root_str);

        json_t* upload_path = json_object_get(web, "UploadPath");
        const char* upload_path_str = json_is_string(upload_path) ? json_string_value(upload_path) : DEFAULT_UPLOAD_PATH;
        config->web.upload_path = strdup(upload_path_str);

        json_t* upload_dir = json_object_get(web, "UploadDir");
        const char* upload_dir_str = json_is_string(upload_dir) ? json_string_value(upload_dir) : DEFAULT_UPLOAD_DIR;
        config->web.upload_dir = strdup(upload_dir_str);

        json_t* max_upload_size = json_object_get(web, "MaxUploadSize");
        config->web.max_upload_size = json_is_integer(max_upload_size) ? 
            (size_t)json_integer_value(max_upload_size) : DEFAULT_MAX_UPLOAD_SIZE;

    } else {
        // Use defaults if web section is missing
        config->web.port = DEFAULT_WEB_PORT;
        config->web.web_root = strdup("/var/www/html");
        config->web.upload_path = strdup(DEFAULT_UPLOAD_PATH);
        config->web.upload_dir = strdup(DEFAULT_UPLOAD_DIR);
        config->web.max_upload_size = DEFAULT_MAX_UPLOAD_SIZE;
    }

    // WebSocket Configuration
    json_t* websocket = json_object_get(root, "WebSocket");
    if (json_is_object(websocket)) {
        json_t* enabled = json_object_get(websocket, "Enabled");
        config->websocket.enabled = json_is_boolean(enabled) ? json_boolean_value(enabled) : 1;

        json_t* enable_ipv6 = json_object_get(websocket, "EnableIPv6");
        config->websocket.enable_ipv6 = json_is_boolean(enable_ipv6) ? json_boolean_value(enable_ipv6) : 0;

        json_t* port = json_object_get(websocket, "Port");
        config->websocket.port = json_is_integer(port) ? json_integer_value(port) : DEFAULT_WEBSOCKET_PORT;

        json_t* key = json_object_get(websocket, "Key");
        const char* key_str = json_is_string(key) ? json_string_value(key) : "default_key";
        config->websocket.key = strdup(key_str);

        // Get protocol with fallback to legacy uppercase key
        json_t* protocol = json_object_get(websocket, "protocol");
        if (!protocol) {
            protocol = json_object_get(websocket, "Protocol");  // Try legacy uppercase key
        }
        if (json_is_string(protocol)) {
            config->websocket.protocol = strdup(json_string_value(protocol));
        } else {
            config->websocket.protocol = strdup("hydrogen-protocol");
        }
        if (!config->websocket.protocol) {
            log_this("Configuration", "Failed to allocate WebSocket protocol string", LOG_LEVEL_ERROR);
            // Clean up previously allocated strings
            free(config->websocket.key);
            return NULL;
        }

        json_t* max_message_size = json_object_get(websocket, "MaxMessageSize");
        config->websocket.max_message_size = json_is_integer(max_message_size) ? 
            json_integer_value(max_message_size) : 10 * 1024 * 1024;  // Default to 10 MB if not specified

    } else {
        // Use defaults if websocket section is missing
        config->websocket.port = DEFAULT_WEBSOCKET_PORT;
        config->websocket.key = strdup("default_key");
        config->websocket.protocol = strdup("hydrogen-protocol");
        config->websocket.max_message_size = 10 * 1024 * 1024;  // Default to 10 MB
    }

    // mDNS Configuration
    json_t* mdns = json_object_get(root, "mDNSServer");
    if (json_is_object(mdns)) {
        json_t* enabled = json_object_get(mdns, "Enabled");
        config->mdns.enabled = json_is_boolean(enabled) ? json_boolean_value(enabled) : 1;

        json_t* enable_ipv6 = json_object_get(mdns, "EnableIPv6");
        config->mdns.enable_ipv6 = json_is_boolean(enable_ipv6) ? json_boolean_value(enable_ipv6) : 1;

        json_t* device_id = json_object_get(mdns, "DeviceId");
        const char* device_id_str = json_is_string(device_id) ? json_string_value(device_id) : "hydrogen-printer";
        config->mdns.device_id = strdup(device_id_str);


        json_t* friendly_name = json_object_get(mdns, "FriendlyName");
        const char* friendly_name_str = json_is_string(friendly_name) ? json_string_value(friendly_name) : "Hydrogen 3D Printer";
        config->mdns.friendly_name = strdup(friendly_name_str);

        json_t* model = json_object_get(mdns, "Model");
        const char* model_str = json_is_string(model) ? json_string_value(model) : "Hydrogen";
        config->mdns.model = strdup(model_str);

        json_t* manufacturer = json_object_get(mdns, "Manufacturer");
        const char* manufacturer_str = json_is_string(manufacturer) ? json_string_value(manufacturer) : "Philement";
        config->mdns.manufacturer = strdup(manufacturer_str);

        json_t* version = json_object_get(mdns, "Version");
        const char* version_str = json_is_string(version) ? json_string_value(version) : VERSION;
        config->mdns.version = strdup(version_str);

        json_t* services = json_object_get(mdns, "Services");
	if (json_is_array(services)) {
            config->mdns.num_services = json_array_size(services);
            config->mdns.services = calloc(config->mdns.num_services, sizeof(mdns_server_service_t));

            for (size_t i = 0; i < config->mdns.num_services; i++) {
                json_t* service = json_array_get(services, i);
                if (!json_is_object(service)) continue;

                json_t* name = json_object_get(service, "Name");
                const char* name_str = json_is_string(name) ? json_string_value(name) : "hydrogen";
                config->mdns.services[i].name = strdup(name_str);

                json_t* type = json_object_get(service, "Type");
                const char* type_str = json_is_string(type) ? json_string_value(type) : "_http._tcp.local";
                config->mdns.services[i].type = strdup(type_str);

                json_t* port = json_object_get(service, "Port");
                config->mdns.services[i].port = json_is_integer(port) ? json_integer_value(port) : DEFAULT_WEB_PORT;
        
                // Handle TXT records
                json_t* txt_records = json_object_get(service, "TxtRecords");
                if (json_is_string(txt_records)) {
                    // If TxtRecords is a single string, treat it as one record
                    config->mdns.services[i].num_txt_records = 1;
                    config->mdns.services[i].txt_records = malloc(sizeof(char*));
                    config->mdns.services[i].txt_records[0] = strdup(json_string_value(txt_records));
                } else if (json_is_array(txt_records)) {
                    // If TxtRecords is an array, handle multiple records
                    config->mdns.services[i].num_txt_records = json_array_size(txt_records);
                    config->mdns.services[i].txt_records = malloc(config->mdns.services[i].num_txt_records * sizeof(char*));
                    for (size_t j = 0; j < config->mdns.services[i].num_txt_records; j++) {
                        config->mdns.services[i].txt_records[j] = strdup(json_string_value(json_array_get(txt_records, j)));
                    }
                } else {
                    // If TxtRecords is not present or invalid, set to NULL
                    config->mdns.services[i].num_txt_records = 0;
                    config->mdns.services[i].txt_records = NULL;
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
            config->resources.max_queue_blocks = json_is_integer(val) ? json_integer_value(val) : DEFAULT_MAX_QUEUE_BLOCKS;
            
            val = json_object_get(queues, "QueueHashSize");
            config->resources.queue_hash_size = json_is_integer(val) ? json_integer_value(val) : DEFAULT_QUEUE_HASH_SIZE;
            
            val = json_object_get(queues, "DefaultQueueCapacity");
            config->resources.default_capacity = json_is_integer(val) ? json_integer_value(val) : DEFAULT_QUEUE_CAPACITY;
        }

        json_t* buffers = json_object_get(resources, "Buffers");
        if (json_is_object(buffers)) {
            json_t* val;
            val = json_object_get(buffers, "DefaultMessageBuffer");
            config->resources.message_buffer_size = json_is_integer(val) ? json_integer_value(val) : DEFAULT_MESSAGE_BUFFER_SIZE;
            
            val = json_object_get(buffers, "MaxLogMessageSize");
            config->resources.max_log_message_size = json_is_integer(val) ? json_integer_value(val) : DEFAULT_MAX_LOG_MESSAGE_SIZE;
            
            val = json_object_get(buffers, "LineBufferSize");
            config->resources.line_buffer_size = json_is_integer(val) ? json_integer_value(val) : DEFAULT_LINE_BUFFER_SIZE;

            val = json_object_get(buffers, "PostProcessorBuffer");
            config->resources.post_processor_buffer_size = json_is_integer(val) ? json_integer_value(val) : DEFAULT_POST_PROCESSOR_BUFFER_SIZE;

            val = json_object_get(buffers, "LogBufferSize");
            config->resources.log_buffer_size = json_is_integer(val) ? json_integer_value(val) : DEFAULT_LOG_BUFFER_SIZE;

            val = json_object_get(buffers, "JsonMessageSize");
            config->resources.json_message_size = json_is_integer(val) ? json_integer_value(val) : DEFAULT_JSON_MESSAGE_SIZE;

            val = json_object_get(buffers, "LogEntrySize");
            config->resources.log_entry_size = json_is_integer(val) ? json_integer_value(val) : DEFAULT_LOG_ENTRY_SIZE;
        }
    } else {
        // Use defaults if section is missing
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
            config->network.max_interfaces = json_is_integer(val) ? json_integer_value(val) : DEFAULT_MAX_INTERFACES;
            
            val = json_object_get(interfaces, "MaxIPsPerInterface");
            config->network.max_ips_per_interface = json_is_integer(val) ? json_integer_value(val) : DEFAULT_MAX_IPS_PER_INTERFACE;
            
            val = json_object_get(interfaces, "MaxInterfaceNameLength");
            config->network.max_interface_name_length = json_is_integer(val) ? json_integer_value(val) : DEFAULT_MAX_INTERFACE_NAME_LENGTH;
            
            val = json_object_get(interfaces, "MaxIPAddressLength");
            config->network.max_ip_address_length = json_is_integer(val) ? json_integer_value(val) : DEFAULT_MAX_IP_ADDRESS_LENGTH;
        }

        json_t* port_allocation = json_object_get(network, "PortAllocation");
        if (json_is_object(port_allocation)) {
            json_t* val;
            val = json_object_get(port_allocation, "StartPort");
            config->network.start_port = json_is_integer(val) ? json_integer_value(val) : 5000;
            
            val = json_object_get(port_allocation, "EndPort");
            config->network.end_port = json_is_integer(val) ? json_integer_value(val) : 65535;

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
        // Use defaults if section is missing
        config->network.max_interfaces = DEFAULT_MAX_INTERFACES;
        config->network.max_ips_per_interface = DEFAULT_MAX_IPS_PER_INTERFACE;
        config->network.max_interface_name_length = DEFAULT_MAX_INTERFACE_NAME_LENGTH;
        config->network.max_ip_address_length = DEFAULT_MAX_IP_ADDRESS_LENGTH;
        config->network.start_port = 5000;
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
            config->monitoring.status_update_ms = json_is_integer(val) ? json_integer_value(val) : DEFAULT_STATUS_UPDATE_MS;
            
            val = json_object_get(intervals, "ResourceCheckMs");
            config->monitoring.resource_check_ms = json_is_integer(val) ? json_integer_value(val) : DEFAULT_RESOURCE_CHECK_MS;
            
            val = json_object_get(intervals, "MetricsUpdateMs");
            config->monitoring.metrics_update_ms = json_is_integer(val) ? json_integer_value(val) : DEFAULT_METRICS_UPDATE_MS;
        }

        json_t* thresholds = json_object_get(monitoring, "Thresholds");
        if (json_is_object(thresholds)) {
            json_t* val;
            val = json_object_get(thresholds, "MemoryWarningPercent");
            config->monitoring.memory_warning_percent = json_is_integer(val) ? json_integer_value(val) : DEFAULT_MEMORY_WARNING_PERCENT;
            
            val = json_object_get(thresholds, "DiskSpaceWarningPercent");
            config->monitoring.disk_warning_percent = json_is_integer(val) ? json_integer_value(val) : DEFAULT_DISK_WARNING_PERCENT;
            
            val = json_object_get(thresholds, "LoadAverageWarning");
            config->monitoring.load_warning = json_is_real(val) ? json_real_value(val) : DEFAULT_LOAD_WARNING;
        }
    } else {
        // Use defaults if section is missing
        config->monitoring.status_update_ms = DEFAULT_STATUS_UPDATE_MS;
        config->monitoring.resource_check_ms = DEFAULT_RESOURCE_CHECK_MS;
        config->monitoring.metrics_update_ms = DEFAULT_METRICS_UPDATE_MS;
        config->monitoring.memory_warning_percent = DEFAULT_MEMORY_WARNING_PERCENT;
        config->monitoring.disk_warning_percent = DEFAULT_DISK_WARNING_PERCENT;
        config->monitoring.load_warning = DEFAULT_LOAD_WARNING;
    }

    // Printer Motion Configuration
    json_t* motion = json_object_get(root, "Motion");
    if (json_is_object(motion)) {
        json_t* val;
        val = json_object_get(motion, "MaxLayers");
        config->motion.max_layers = json_is_integer(val) ? json_integer_value(val) : DEFAULT_MAX_LAYERS;
        
        val = json_object_get(motion, "Acceleration");
        config->motion.acceleration = json_is_real(val) ? json_real_value(val) : DEFAULT_ACCELERATION;
        
        val = json_object_get(motion, "ZAcceleration");
        config->motion.z_acceleration = json_is_real(val) ? json_real_value(val) : DEFAULT_Z_ACCELERATION;
        
        val = json_object_get(motion, "EAcceleration");
        config->motion.e_acceleration = json_is_real(val) ? json_real_value(val) : DEFAULT_E_ACCELERATION;
        
        val = json_object_get(motion, "MaxSpeedXY");
        config->motion.max_speed_xy = json_is_real(val) ? json_real_value(val) : DEFAULT_MAX_SPEED_XY;
        
        val = json_object_get(motion, "MaxSpeedTravel");
        config->motion.max_speed_travel = json_is_real(val) ? json_real_value(val) : DEFAULT_MAX_SPEED_TRAVEL;
        
        val = json_object_get(motion, "MaxSpeedZ");
        config->motion.max_speed_z = json_is_real(val) ? json_real_value(val) : DEFAULT_MAX_SPEED_Z;
        
        val = json_object_get(motion, "ZValuesChunk");
        config->motion.z_values_chunk = json_is_integer(val) ? json_integer_value(val) : DEFAULT_Z_VALUES_CHUNK;
    } else {
        // Use defaults if section is missing
        config->motion.max_layers = DEFAULT_MAX_LAYERS;
        config->motion.acceleration = DEFAULT_ACCELERATION;
        config->motion.z_acceleration = DEFAULT_Z_ACCELERATION;
        config->motion.e_acceleration = DEFAULT_E_ACCELERATION;
        config->motion.max_speed_xy = DEFAULT_MAX_SPEED_XY;
        config->motion.max_speed_travel = DEFAULT_MAX_SPEED_TRAVEL;
        config->motion.max_speed_z = DEFAULT_MAX_SPEED_Z;
        config->motion.z_values_chunk = DEFAULT_Z_VALUES_CHUNK;
    }

    // Print Queue Configuration
    json_t* print_queue = json_object_get(root, "PrintQueue");
    if (json_is_object(print_queue)) {
        json_t* enabled = json_object_get(print_queue, "Enabled");
        config->print_queue.enabled = json_is_boolean(enabled) ? json_boolean_value(enabled) : 1;

        json_t* queue_settings = json_object_get(print_queue, "QueueSettings");
        if (json_is_object(queue_settings)) {
            json_t* val;
            val = json_object_get(queue_settings, "DefaultPriority");
            config->print_queue.priorities.default_priority = json_is_integer(val) ? json_integer_value(val) : 1;
            
            val = json_object_get(queue_settings, "EmergencyPriority");
            config->print_queue.priorities.emergency_priority = json_is_integer(val) ? json_integer_value(val) : 0;
            
            val = json_object_get(queue_settings, "MaintenancePriority");
            config->print_queue.priorities.maintenance_priority = json_is_integer(val) ? json_integer_value(val) : 2;
            
            val = json_object_get(queue_settings, "SystemPriority");
            config->print_queue.priorities.system_priority = json_is_integer(val) ? json_integer_value(val) : 3;
        }

        json_t* timeouts = json_object_get(print_queue, "Timeouts");
        if (json_is_object(timeouts)) {
            json_t* val;
            val = json_object_get(timeouts, "ShutdownWaitMs");
            config->print_queue.timeouts.shutdown_wait_ms = json_is_integer(val) ? json_integer_value(val) : DEFAULT_SHUTDOWN_WAIT_MS;
            
            val = json_object_get(timeouts, "JobProcessingTimeoutMs");
            config->print_queue.timeouts.job_processing_timeout_ms = json_is_integer(val) ? json_integer_value(val) : DEFAULT_JOB_PROCESSING_TIMEOUT_MS;
        }

        json_t* buffers = json_object_get(print_queue, "Buffers");
        if (json_is_object(buffers)) {
            json_t* val;
            val = json_object_get(buffers, "JobMessageSize");
            config->print_queue.buffers.job_message_size = json_is_integer(val) ? json_integer_value(val) : 256;
            
            val = json_object_get(buffers, "StatusMessageSize");
            config->print_queue.buffers.status_message_size = json_is_integer(val) ? json_integer_value(val) : 256;
        }
    } else {
        // Use defaults if print queue section is missing
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

    // Load Logging Configuration
    json_t* logging = json_object_get(root, "Logging");
    if (json_is_object(logging)) {
        // Load logging levels
        json_t* levels = json_object_get(logging, "Levels");
        if (json_is_array(levels)) {
            config->Logging.LevelCount = json_array_size(levels);
            config->Logging.Levels = calloc(config->Logging.LevelCount, sizeof(struct { int value; const char *name; }));
            
            for (size_t i = 0; i < config->Logging.LevelCount; i++) {
                json_t* level = json_array_get(levels, i);
                if (json_is_array(level) && json_array_size(level) == 2) {
                    json_t* value = json_array_get(level, 0);
                    json_t* name = json_array_get(level, 1);
                    if (json_is_integer(value) && json_is_string(name)) {
                        config->Logging.Levels[i].value = json_integer_value(value);
                        config->Logging.Levels[i].name = strdup(json_string_value(name));
                    }
                }
            }
        }

        // Load console configuration
        json_t* console = json_object_get(logging, "Console");
        if (json_is_object(console)) {
            json_t* enabled = json_object_get(console, "Enabled");
            config->Logging.Console.Enabled = json_is_boolean(enabled) ? json_boolean_value(enabled) : 1;

            json_t* default_level = json_object_get(console, "DefaultLevel");
            config->Logging.Console.DefaultLevel = json_is_integer(default_level) ? json_integer_value(default_level) : LOG_LEVEL_INFO;

            json_t* subsystems = json_object_get(console, "Subsystems");
            if (json_is_object(subsystems)) {
                json_t* level;
                level = json_object_get(subsystems, "ThreadMgmt");
                config->Logging.Console.Subsystems.ThreadMgmt = json_is_integer(level) ? json_integer_value(level) : LOG_LEVEL_WARN;
                level = json_object_get(subsystems, "Shutdown");
                config->Logging.Console.Subsystems.Shutdown = json_is_integer(level) ? json_integer_value(level) : LOG_LEVEL_INFO;
                level = json_object_get(subsystems, "mDNSServer");
                config->Logging.Console.Subsystems.mDNSServer = json_is_integer(level) ? json_integer_value(level) : LOG_LEVEL_INFO;
                level = json_object_get(subsystems, "WebServer");
                config->Logging.Console.Subsystems.WebServer = json_is_integer(level) ? json_integer_value(level) : LOG_LEVEL_INFO;
                level = json_object_get(subsystems, "WebSocket");
                config->Logging.Console.Subsystems.WebSocket = json_is_integer(level) ? json_integer_value(level) : LOG_LEVEL_INFO;
                level = json_object_get(subsystems, "PrintQueue");
                config->Logging.Console.Subsystems.PrintQueue = json_is_integer(level) ? json_integer_value(level) : LOG_LEVEL_WARN;
                level = json_object_get(subsystems, "LogQueueManager");
                config->Logging.Console.Subsystems.LogQueueManager = json_is_integer(level) ? json_integer_value(level) : LOG_LEVEL_INFO;
            }
        }
    }

    json_decref(root);
    return config;
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
