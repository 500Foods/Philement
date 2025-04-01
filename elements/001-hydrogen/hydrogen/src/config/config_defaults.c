/*
 * Default configuration generation with secure baselines
 * 
 * Why This Architecture:
 * 1. Modularity
 *    - Each subsystem has dedicated generator
 *    - Clear separation of concerns
 *    - Easy to maintain and extend
 * 
 * 2. Security
 *    - Conservative defaults
 *    - Resource limits
 *    - Access controls
 *    - Secure protocols
 * 
 * 3. Reliability
 *    - Memory management
 *    - Error handling
 *    - Resource cleanup
 *    - Consistent state
 * 
 * Thread Safety:
 * - Functions are not thread-safe
 * - Should only be called during initialization
 * - No shared state between calls
 */

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>

// Project headers
#include "config_defaults.h"
#include "config.h"
#include "../logging/logging.h"

/*
 * Generate default server configuration
 * 
 * Why these defaults?
 * - Core system identification
 * - Essential paths and locations
 * - Security settings
 * - Runtime behavior controls
 * 
 * Environment Variables:
 * - HYDROGEN_SERVER_NAME: Override server name
 * - HYDROGEN_LOG_PATH: Override log file location
 * - HYDROGEN_PAYLOAD_KEY: Required for payload decryption
 * - HYDROGEN_STARTUP_DELAY: Override startup delay (ms)
 * - HYDROGEN_CONFIG_DIR: Override config directory
 * - HYDROGEN_DATA_DIR: Override data directory
 * - HYDROGEN_TEMP_DIR: Override temp directory
 */
json_t* create_default_server_config(void) {
    json_t* server = json_object();
    if (!server) {
        log_this("Configuration", "Failed to create server config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    // Core Identity
    json_object_set_new(server, "ServerName", json_string("${env.HYDROGEN_SERVER_NAME:-Philement/hydrogen}"));
    json_object_set_new(server, "Version", json_string(VERSION));
    json_object_set_new(server, "BuildType", json_string("${env.BUILD_TYPE:-release}"));

    // Essential Paths
    json_object_set_new(server, "LogFile", json_string("${env.HYDROGEN_LOG_PATH:-/var/log/hydrogen.log}"));
    json_object_set_new(server, "ConfigDir", json_string("${env.HYDROGEN_CONFIG_DIR:-/etc/hydrogen}"));
    json_object_set_new(server, "DataDir", json_string("${env.HYDROGEN_DATA_DIR:-/var/lib/hydrogen}"));
    json_object_set_new(server, "TempDir", json_string("${env.HYDROGEN_TEMP_DIR:-/tmp/hydrogen}"));

    // Security
    json_object_set_new(server, "PayloadKey", json_string("${env.HYDROGEN_PAYLOAD_KEY}"));
    json_object_set_new(server, "FileMode", json_integer(0640));  // rw-r----- default file permissions
    json_object_set_new(server, "DirMode", json_integer(0750));   // rwxr-x--- default directory permissions

    // Runtime Behavior
    json_t* startup = json_object();
    json_object_set_new(startup, "DelayMs", json_string("${env.HYDROGEN_STARTUP_DELAY:-5000}"));
    json_object_set_new(startup, "MaxAttempts", json_integer(3));
    json_object_set_new(startup, "RetryDelayMs", json_integer(1000));
    json_object_set_new(server, "Startup", startup);

    json_t* shutdown = json_object();
    json_object_set_new(shutdown, "GracePeriodMs", json_integer(5000));
    json_object_set_new(shutdown, "ForceTimeoutMs", json_integer(10000));
    json_object_set_new(server, "Shutdown", shutdown);

    // Resource Limits
    json_t* limits = json_object();
    json_object_set_new(limits, "MaxLogSize", json_integer(100 * 1024 * 1024));  // 100MB
    json_object_set_new(limits, "MaxConfigSize", json_integer(1 * 1024 * 1024)); // 1MB
    json_object_set_new(limits, "MaxTempAge", json_integer(86400));              // 24 hours
    json_object_set_new(server, "Limits", limits);

    return server;
}

/*
 * Generate default web server configuration
 * 
 * Why these defaults?
 * - Standard ports for easy discovery
 * - Conservative upload limits
 * - Secure file permissions
 * - Clear API structure
 */
json_t* create_default_web_config(void) {
    json_t* web = json_object();
    if (!web) {
        log_this("Configuration", "Failed to create web server config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_object_set_new(web, "Enabled", json_boolean(1));
    json_object_set_new(web, "EnableIPv6", json_boolean(0));
    json_object_set_new(web, "Port", json_integer(DEFAULT_WEB_PORT));
    json_object_set_new(web, "WebRoot", json_string("/var/www/html"));
    json_object_set_new(web, "UploadPath", json_string(DEFAULT_UPLOAD_PATH));
    json_object_set_new(web, "UploadDir", json_string(DEFAULT_UPLOAD_DIR));
    json_object_set_new(web, "MaxUploadSize", json_integer(DEFAULT_MAX_UPLOAD_SIZE));

    return web;
}

/*
 * Generate default WebSocketServer configuration
 * 
 * Why these defaults?
 * - Secure protocol settings
 * - Standard ports
 * - Conservative message limits
 * - Clear timeouts
 */
json_t* create_default_websocket_config(void) {
    json_t* websocket = json_object();
    if (!websocket) {
        log_this("Configuration", "Failed to create WebSocketServer config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_object_set_new(websocket, "Enabled", json_boolean(DEFAULT_WEBSOCKET_ENABLED));
    json_object_set_new(websocket, "EnableIPv6", json_boolean(DEFAULT_WEBSOCKET_ENABLE_IPV6));
    json_object_set_new(websocket, "Port", json_integer(DEFAULT_WEBSOCKET_PORT));
    json_object_set_new(websocket, "Key", json_string(DEFAULT_WEBSOCKET_KEY));
    json_object_set_new(websocket, "Protocol", json_string(DEFAULT_WEBSOCKET_PROTOCOL));
    json_object_set_new(websocket, "MaxMessageSize", json_integer(10 * 1024 * 1024));

    json_t* timeouts = json_object();
    json_object_set_new(timeouts, "ExitWaitSeconds", json_integer(10));
    json_object_set_new(websocket, "ConnectionTimeouts", timeouts);

    return websocket;
}

/*
 * Generate default mDNS server configuration
 * 
 * Why these defaults?
 * - Standard service discovery
 * - Clear device identification
 * - Multiple service types
 * - Discoverable ports
 */
json_t* create_default_mdns_config(void) {
    json_t* mdns = json_object();
    if (!mdns) {
        log_this("Configuration", "Failed to create mDNS config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_object_set_new(mdns, "Enabled", json_boolean(1));
    json_object_set_new(mdns, "EnableIPv6", json_boolean(0));
    json_object_set_new(mdns, "DeviceId", json_string("hydrogen-printer"));
    json_object_set_new(mdns, "FriendlyName", json_string("Hydrogen 3D Printer"));
    json_object_set_new(mdns, "Model", json_string("Hydrogen"));
    json_object_set_new(mdns, "Manufacturer", json_string("Philement"));
    json_object_set_new(mdns, "Version", json_string(VERSION));

    json_t* services = json_array();
    
    // HTTP service
    json_t* http = json_object();
    json_object_set_new(http, "Name", json_string("hydrogen"));
    json_object_set_new(http, "Type", json_string("_http._tcp.local"));
    json_object_set_new(http, "Port", json_integer(DEFAULT_WEB_PORT));
    json_object_set_new(http, "TxtRecords", json_string("path=/api/upload"));
    json_array_append_new(services, http);

    // WebSocket service
    json_t* ws = json_object();
    json_object_set_new(ws, "Name", json_string("Hydrogen"));
    json_object_set_new(ws, "Type", json_string("_websocket._tcp.local"));
    json_object_set_new(ws, "Port", json_integer(DEFAULT_WEBSOCKET_PORT));
    json_object_set_new(ws, "TxtRecords", json_string("path=/websocket"));
    json_array_append_new(services, ws);

    json_object_set_new(mdns, "Services", services);
    return mdns;
}

/*
 * Generate default system resources configuration
 * 
 * Why these defaults?
 * - Conservative memory usage
 * - Reasonable queue sizes
 * - Safe buffer limits
 * - Clear resource boundaries
 */
json_t* create_default_resources_config(void) {
    json_t* resources = json_object();
    if (!resources) {
        log_this("Configuration", "Failed to create resources config object", LOG_LEVEL_ERROR);
        return NULL;
    }

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

    return resources;
}

/*
 * Generate default network configuration
 * 
 * Why these defaults?
 * - Standard interface limits
 * - Safe port ranges
 * - Reserved port protection
 * - Clear boundaries
 */
json_t* create_default_network_config(void) {
    json_t* network = json_object();
    if (!network) {
        log_this("Configuration", "Failed to create network config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_t* interfaces = json_object();
    json_object_set_new(interfaces, "MaxInterfaces", json_integer(DEFAULT_MAX_INTERFACES));
    json_object_set_new(interfaces, "MaxIPsPerInterface", json_integer(DEFAULT_MAX_IPS_PER_INTERFACE));
    json_object_set_new(interfaces, "MaxInterfaceNameLength", json_integer(DEFAULT_MAX_INTERFACE_NAME_LENGTH));
    json_object_set_new(interfaces, "MaxIPAddressLength", json_integer(DEFAULT_MAX_IP_ADDRESS_LENGTH));
    json_object_set_new(network, "Interfaces", interfaces);

    json_t* ports = json_object();
    json_object_set_new(ports, "StartPort", json_integer(DEFAULT_WEB_PORT));
    json_object_set_new(ports, "EndPort", json_integer(65535));
    
    json_t* reserved = json_array();
    json_array_append_new(reserved, json_integer(22));   // SSH
    json_array_append_new(reserved, json_integer(80));   // HTTP
    json_array_append_new(reserved, json_integer(443));  // HTTPS
    json_object_set_new(ports, "ReservedPorts", reserved);
    
    json_object_set_new(network, "PortAllocation", ports);

    return network;
}

/*
 * Generate default system monitoring configuration
 * 
 * Why these defaults?
 * - Regular status updates
 * - Resource monitoring
 * - Warning thresholds
 * - Performance metrics
 */
json_t* create_default_monitoring_config(void) {
    json_t* monitoring = json_object();
    if (!monitoring) {
        log_this("Configuration", "Failed to create monitoring config object", LOG_LEVEL_ERROR);
        return NULL;
    }

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

    return monitoring;
}

/*
 * Generate default print queue configuration
 * 
 * Why these defaults?
 * - Priority system
 * - Safe timeouts
 * - Message buffers
 * - Queue management
 */
json_t* create_default_print_queue_config(void) {
    json_t* print_queue = json_object();
    if (!print_queue) {
        log_this("Configuration", "Failed to create print queue config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_object_set_new(print_queue, "Enabled", json_boolean(1));

    json_t* settings = json_object();
    json_object_set_new(settings, "DefaultPriority", json_integer(1));
    json_object_set_new(settings, "EmergencyPriority", json_integer(0));
    json_object_set_new(settings, "MaintenancePriority", json_integer(2));
    json_object_set_new(settings, "SystemPriority", json_integer(3));
    json_object_set_new(print_queue, "QueueSettings", settings);

    json_t* timeouts = json_object();
    json_object_set_new(timeouts, "ShutdownWaitMs", json_integer(DEFAULT_SHUTDOWN_WAIT_MS));
    json_object_set_new(timeouts, "JobProcessingTimeoutMs", json_integer(DEFAULT_JOB_PROCESSING_TIMEOUT_MS));
    json_object_set_new(print_queue, "Timeouts", timeouts);

    json_t* buffers = json_object();
    json_object_set_new(buffers, "JobMessageSize", json_integer(256));
    json_object_set_new(buffers, "StatusMessageSize", json_integer(256));
    json_object_set_new(print_queue, "Buffers", buffers);

    return print_queue;
}

/*
 * Generate default API configuration
 * 
 * Why these defaults?
 * - Secure tokens
 * - Clear warning about defaults
 * - Production-ready settings
 * - Security first
 */
json_t* create_default_api_config(void) {
    json_t* api = json_object();
    if (!api) {
        log_this("Configuration", "Failed to create API config object", LOG_LEVEL_ERROR);
        return NULL;
    }

    json_object_set_new(api, "JWTSecret", json_string("hydrogen_api_secret_change_me"));

    return api;
}

/*
 * Generate complete default configuration
 * 
 * Why this approach?
 * 1. Modularity
 *    - Each subsystem handled separately
 *    - Clear error boundaries
 *    - Independent generation
 * 
 * 2. Error Handling
 *    - Cleanup on any failure
 *    - Clear error reporting
 *    - Safe defaults
 * 
 * 3. Maintainability
 *    - Easy to extend
 *    - Clear structure
 *    - Documented defaults
 */
void create_default_config(const char* config_path) {
    json_t* root = json_object();
    if (!root) {
        log_this("Configuration", "Failed to create root config object", LOG_LEVEL_ERROR);
        return;
    }

    // Generate server configuration first (registry priority)
    json_t* server = create_default_server_config();
    if (server) json_object_set_new(root, "Server", server);

    // Generate other subsystem configurations
    json_t* web = create_default_web_config();
    json_t* websocket = create_default_websocket_config();
    json_t* mdns = create_default_mdns_config();
    json_t* resources = create_default_resources_config();
    json_t* network = create_default_network_config();
    json_t* monitoring = create_default_monitoring_config();
    json_t* print_queue = create_default_print_queue_config();
    json_t* api = create_default_api_config();

    // Add subsystems to root (with error checking)
    if (web) json_object_set_new(root, "WebServer", web);
    if (websocket) json_object_set_new(root, "WebSocketServer", websocket);
    if (mdns) json_object_set_new(root, "mDNSServer", mdns);
    if (resources) json_object_set_new(root, "SystemResources", resources);
    if (network) json_object_set_new(root, "Network", network);
    if (monitoring) json_object_set_new(root, "SystemMonitoring", monitoring);
    if (print_queue) json_object_set_new(root, "PrintQueue", print_queue);
    if (api) json_object_set_new(root, "API", api);

    // Write configuration to file
    if (json_dump_file(root, config_path, JSON_INDENT(4)) != 0) {
        log_this("Configuration", "Error: Unable to create default config at %s", 
                 LOG_LEVEL_ERROR, config_path);
    } else {
        log_this("Configuration", "Created default config at %s", 
                 LOG_LEVEL_STATE, config_path);
    }

    json_decref(root);
}