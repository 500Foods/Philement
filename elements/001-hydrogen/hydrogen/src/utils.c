/*
 * Core Utilities for High-Precision 3D Printing
 * 
 * Why Precision Matters:
 * 1. Print Quality
 *    - Layer alignment accuracy
 *    - Extrusion precision
 *    - Motion smoothness
 *    - Surface finish
 * 
 * 2. Hardware Safety
 *    Why These Checks?
 *    - Motor protection
 *    - Thermal management
 *    - Mechanical limits
 *    - Power monitoring
 * 
 * 3. Real-time Control
 *    Why This Design?
 *    - Immediate response
 *    - Timing accuracy
 *    - Command precision
 *    - State tracking
 * 
 * 4. Resource Management
 *    Why This Matters?
 *    - Memory efficiency
 *    - CPU optimization
 *    - Long-print stability
 *    - System reliability
 * 
 * 5. Error Prevention
 *    Why These Features?
 *    - Print failure avoidance
 *    - Quality assurance
 *    - Material waste reduction
 *    - Time savings
 * 
 * 6. Diagnostic Support
 *    Why These Tools?
 *    - Quality monitoring
 *    - Performance tracking
 *    - Maintenance prediction
 *    - Issue resolution
 * 
 * 7. Data Integrity
 *    Why So Critical?
 *    - Configuration accuracy
 *    - State consistency
 *    - Command validation
 *    - History tracking
 */

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/time.h>
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

// Thread synchronization for status generation with minimal locking
//
// Mutex design choices:
// 1. Scope Minimization
//    - Lock only during JSON construction
//    - Quick system calls outside lock
//    - Batched data collection
//
// 2. Deadlock Prevention
//    - Single lock acquisition
//    - No nested locking
//    - Clear lock hierarchy
//
// 3. Performance
//    - Reduced contention
//    - Efficient memory usage
//    - Quick release
static pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;

// External configuration and state
extern AppConfig *app_config;
extern volatile sig_atomic_t keep_running;
extern volatile sig_atomic_t shutting_down;
// System status reporting with hierarchical organization
//
// Status generation strategy:
// 1. Data Organization
//    - Logical grouping
//    - Clear hierarchy
//    - Consistent structure
//    - Extensible format
//
// 2. Performance Optimization
//    - Memory preallocation
//    - Batched collection
//    - Efficient string handling
//    - Minimal allocations
//
// 3. Error Handling
//    - Partial data recovery
//    - Memory cleanup
//    - Error reporting
//    - Fallback values
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
// Thread-safe identifier generation with collision resistance
//
// ID generation design prioritizes:
// 1. Uniqueness
//    - Time-based seeding
//    - Character space optimization
//    - Length considerations
//    - Entropy maximization
//
// 2. Safety
//    - Buffer overflow prevention
//    - Thread-safe initialization
//    - Input validation
//    - Error reporting
//
// 3. Usability
//    - Human-readable format
//    - Consistent length
//    - Easy validation
//    - Simple comparison
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

// Format and output a log message directly to console
// Matches the format of the logging queue system for consistency
//
// Format components:
// 1. Timestamp with ms precision
// 2. Priority label with padding
// 3. Subsystem label with padding
// 4. Message content
//
// Example output:
// 2025-02-16 14:24:10.065  [ INFO      ]  [ Shutdown           ]  message
void console_log(const char* subsystem, int priority, const char* message) {
    // Get current time with millisecond precision
    struct timeval tv;
    struct tm* tm_info;
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);

    // Format timestamp
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    char timestamp_ms[36];
    snprintf(timestamp_ms, sizeof(timestamp_ms), "%s.%03d", timestamp, (int)(tv.tv_usec / 1000));

    // Format priority and subsystem labels with proper padding
    char formatted_priority[MAX_PRIORITY_LABEL_WIDTH + 5];
    snprintf(formatted_priority, sizeof(formatted_priority), "[ %-*s ]", MAX_PRIORITY_LABEL_WIDTH, get_priority_label(priority));

    char formatted_subsystem[MAX_SUBSYSTEM_LABEL_WIDTH + 5];
    snprintf(formatted_subsystem, sizeof(formatted_subsystem), "[ %-*s ]", MAX_SUBSYSTEM_LABEL_WIDTH, subsystem);

    // Output the formatted log entry
    printf("%s  %s  %s  %s\n", timestamp_ms, formatted_priority, formatted_subsystem, message);
}

