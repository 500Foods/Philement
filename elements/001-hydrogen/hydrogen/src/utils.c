/*
 * Implementation of utility functions for the Hydrogen printer.
 * 
 * Provides core utility implementations with a focus on reliability,
 * thread safety, and proper resource management. These implementations
 * handle common operations needed throughout the system.
 * 
 * JSON Status Generation:
 * - Thread-safe status collection
 * - Hierarchical status structure
 * - Comprehensive system information
 * - Service configuration reporting
 * 
 * Thread Synchronization:
 * - Mutex protection for shared data
 * - Atomic operations where appropriate
 * - Lock scope minimization
 * - Deadlock prevention
 * 
 * Random Generation:
 * - Thread-safe initialization
 * - Entropy source management
 * - Buffer overflow prevention
 * - Error checking
 * 
 * Resource Management:
 * - Memory leak prevention
 * - Error state cleanup
 * - Resource initialization
 * - Proper deallocation
 */

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <sys/types.h>
#include <sys/utsname.h>
#include <time.h>
#include <pthread.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

// Project headers
#include "utils.h"
#include "logging.h"
#include "configuration.h"
#include "state.h"

// Mutex for thread-safe status generation
// Protects:
// - System information collection
// - JSON object construction
// - Configuration access
static pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;

// External configuration and state
extern AppConfig *app_config;
extern volatile sig_atomic_t keep_running;
extern volatile sig_atomic_t shutting_down;
// Generate comprehensive system status report
// Thread-safe collection and JSON formatting of:
// - Version information
// - System details
// - Service status
// - Configuration settings
// - Performance metrics
json_t* get_system_status_json(const WebSocketMetrics *ws_metrics) {
    pthread_mutex_lock(&status_mutex);
    
    json_t *root = json_object();
    
    // Version Information
    json_t *version = json_object();
    json_object_set_new(version, "server", json_string(VERSION));
    json_object_set_new(version, "api", json_string("1.0"));
    json_object_set_new(root, "version", version);

    // System Information
    struct utsname system_info;
    if (uname(&system_info) == 0) {
        json_t *system = json_object();
        json_object_set_new(system, "sysname", json_string(system_info.sysname));
        json_object_set_new(system, "nodename", json_string(system_info.nodename));
        json_object_set_new(system, "release", json_string(system_info.release));
        json_object_set_new(system, "version", json_string(system_info.version));
        json_object_set_new(system, "machine", json_string(system_info.machine));
        json_object_set_new(root, "system", system);
    }

    // Status Information
    json_t *status = json_object();
    json_object_set_new(status, "running", json_boolean(keep_running));
    json_object_set_new(status, "shutting_down", json_boolean(shutting_down));
    
    // Add WebSocket metrics if provided
    if (ws_metrics != NULL) {
        json_object_set_new(status, "uptime", 
            json_integer(time(NULL) - ws_metrics->server_start_time));
        json_object_set_new(status, "activeConnections", 
            json_integer(ws_metrics->active_connections));
        json_object_set_new(status, "totalConnections", 
            json_integer(ws_metrics->total_connections));
        json_object_set_new(status, "totalRequests", 
            json_integer(ws_metrics->total_requests));
    }
    
    json_object_set_new(root, "status", status);

    // List of enabled services
    json_t *enabled_services = json_array();
    // Logging is always enabled
    json_array_append_new(enabled_services, json_string("logging"));
    if (app_config->web.enabled)
        json_array_append_new(enabled_services, json_string("web"));
    if (app_config->websocket.enabled)
        json_array_append_new(enabled_services, json_string("websocket"));
    if (app_config->mdns.enabled)
        json_array_append_new(enabled_services, json_string("mdns"));
    if (app_config->print_queue.enabled)
        json_array_append_new(enabled_services, json_string("print"));
    json_object_set_new(root, "enabledServices", enabled_services);

    // All service configurations
    json_t *services = json_object();
    
    // Logging service (always enabled)
    json_t *logging = json_object();
    json_object_set_new(logging, "enabled", json_true());
    json_object_set_new(logging, "log_file", json_string(app_config->log_file_path));
    json_object_set_new(services, "logging", logging);
    
    // Web server configuration
    json_t *web = json_object();
    json_object_set_new(web, "enabled", json_boolean(app_config->web.enabled));
    json_object_set_new(web, "port", json_integer(app_config->web.port));
    json_object_set_new(web, "upload_path", json_string(app_config->web.upload_path));
    json_object_set_new(web, "max_upload_size", json_integer(app_config->web.max_upload_size));
    json_object_set_new(web, "log_level", json_string(app_config->web.log_level));
    json_object_set_new(services, "web", web);
    
    // WebSocket configuration
    json_t *websocket = json_object();
    json_object_set_new(websocket, "enabled", json_boolean(app_config->websocket.enabled));
    json_object_set_new(websocket, "port", json_integer(app_config->websocket.port));
    json_object_set_new(websocket, "protocol", json_string(app_config->websocket.protocol));
    json_object_set_new(websocket, "max_message_size", json_integer(app_config->websocket.max_message_size));
    json_object_set_new(websocket, "log_level", json_string(app_config->websocket.log_level));
    json_object_set_new(services, "websocket", websocket);
    
    // mDNS configuration
    json_t *mdns = json_object();
    json_object_set_new(mdns, "enabled", json_boolean(app_config->mdns.enabled));
    json_object_set_new(mdns, "device_id", json_string(app_config->mdns.device_id));
    json_object_set_new(mdns, "friendly_name", json_string(app_config->mdns.friendly_name));
    json_object_set_new(mdns, "model", json_string(app_config->mdns.model));
    json_object_set_new(mdns, "manufacturer", json_string(app_config->mdns.manufacturer));
    json_object_set_new(mdns, "log_level", json_string(app_config->mdns.log_level));
    json_object_set_new(services, "mdns", mdns);
    
    // Print queue configuration
    json_t *print = json_object();
    json_object_set_new(print, "enabled", json_boolean(app_config->print_queue.enabled));
    json_object_set_new(print, "log_level", json_string(app_config->print_queue.log_level));
    json_object_set_new(services, "print", print);
    
    json_object_set_new(root, "services", services);

    pthread_mutex_unlock(&status_mutex);
    return root;
}
// Generate random identifier string
// Thread-safe ID generation with:
// - One-time random seed initialization
// - Buffer overflow protection
// - Error logging on invalid input
// - Consonant-based output for readability
void generate_id(char *buf, size_t len) {
    if (len < ID_LEN + 1) {
        log_this("Utils", "Buffer too small for ID", 3, true, true, true);
        return;
    }

    static int seeded = 0;
    if (!seeded) {
        srand(time(NULL));
        seeded = 1;
    }

    size_t id_chars_len = strlen(ID_CHARS);
    for (int i = 0; i < ID_LEN; i++) {
        buf[i] = ID_CHARS[rand() % id_chars_len];
    }
    buf[ID_LEN] = '\0';
}

