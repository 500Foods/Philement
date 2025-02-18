/*
 * Configuration system for the Hydrogen Project server.
 * 
 * This module implements a hierarchical configuration system that manages all aspects
 * of the Hydrogen server's operation. It's designed to be:
 * 
 * 1. Resilient: Gracefully handles missing or invalid configuration by falling back
 *    to safe defaults. This ensures the server can start even with minimal config.
 * 
 * 2. Flexible: Supports runtime configuration changes and environment-specific
 *    overrides through a JSON-based configuration format.
 * 
 * 3. Secure: Implements careful validation of all inputs, especially paths and
 *    network settings, to prevent security issues.
 * 
 * 4. Maintainable: Uses a structured approach to configuration with clear separation
 *    of concerns between different server components.
 */

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

/*
 * Include Organization:
 * - System headers: Core C library and system-specific headers
 * - Third-party libraries: External dependencies like JSON parsing
 * - Project headers: Internal module dependencies
 */

// System Libraries
#include <stddef.h>

// Third-Party Libraries
#include <jansson.h>

// Project Libraries
#include "mdns_server.h"

/*
 * System-wide Constants:
 * - VERSION: Used for compatibility checking and service advertisement
 * - Defaults: Safe fallback values when config items are missing
 * - Limits: System constraints and safety boundaries
 */
#define VERSION "0.1.0"
#define DEFAULT_SERVER_NAME "Philement/hydrogen"
#define DEFAULT_LOG_FILE "/var/log/hydrogen.log"
#define DEFAULT_WEB_PORT 5000              // Standard HTTP port range
#define DEFAULT_WEBSOCKET_PORT 5001        // Separate from HTTP for clean separation
#define DEFAULT_UPLOAD_PATH "/api/upload"  // REST-style API path
#define DEFAULT_UPLOAD_DIR "/tmp/hydrogen_uploads"  // Secure temp directory
#define DEFAULT_MAX_UPLOAD_SIZE (2ULL * 1024 * 1024 * 1024)  // 2GB limit for safety
#define NUM_PRIORITY_LEVELS 5              // Standard logging levels

/*
 * Logging Priority System:
 * Maps numeric priority levels to human-readable labels for consistent
 * log formatting and filtering across all server components.
 */
typedef struct {
    int value;          // Numeric priority for comparison
    const char* label;  // Human-readable label for display
} PriorityLevel;

/*
 * Web Server Configuration:
 * Controls the HTTP server component that handles REST API requests,
 * file uploads, and static content serving. Includes safety limits
 * and path configurations.
 */
typedef struct {
    int enabled;            // Runtime toggle for web server
    int enable_ipv6;        // IPv6 support toggle
    int port;              // HTTP service port
    char *web_root;        // Static content directory
    char *upload_path;     // URL path for file uploads
    char *upload_dir;      // Storage location for uploads
    size_t max_upload_size;// Upload size limit for DoS prevention
    char *log_level;       // Component-specific logging control
} WebConfig;

/*
 * WebSocket Configuration:
 * Manages real-time bidirectional communication settings for
 * printer status updates and command streaming. Includes
 * security and performance parameters.
 */
typedef struct {
    int enabled;            // Runtime toggle for WebSocket server
    int enable_ipv6;        // IPv6 support toggle
    int port;              // WebSocket service port
    char *key;             // Authentication key
    char *protocol;        // WebSocket subprotocol identifier
    size_t max_message_size;// Message size limit for memory safety
    char *log_level;       // Component-specific logging control
} WebSocketConfig;

/*
 * mDNS Service Discovery Configuration:
 * Enables automatic printer discovery on local networks through
 * multicast DNS. Supports multiple service advertisements for
 * different protocols (HTTP, WebSocket, etc.).
 */
typedef struct {
    int enabled;            // Runtime toggle for mDNS
    int enable_ipv6;        // IPv6 support toggle
    char *device_id;        // Unique device identifier
    char *friendly_name;    // Human-readable device name
    char *model;           // Device model information
    char *manufacturer;    // Manufacturer information
    char *version;        // Firmware/software version
    mdns_service_t *services; // Array of advertised services
    size_t num_services;   // Number of advertised services
    char *log_level;       // Component-specific logging control
} mDNSConfig;

/*
 * Print Queue Configuration:
 * Controls the print job management system, including job
 * scheduling, status tracking, and error handling.
 */
typedef struct {
    int enabled;            // Runtime toggle for print queue
    char *log_level;       // Component-specific logging control
} PrintQueueConfig;

/*
 * Main Application Configuration:
 * Top-level configuration structure that aggregates all component
 * configurations. This represents the complete runtime configuration
 * of the Hydrogen server.
 */
typedef struct {
    char *server_name;      // Server identification
    char *executable_path;  // Binary location for resource loading
    char *log_file_path;   // Central log file location
    WebConfig web;         // HTTP server settings
    WebSocketConfig websocket; // WebSocket server settings
    mDNSConfig mdns;       // Service discovery settings
    PrintQueueConfig print_queue; // Print management settings
} AppConfig;

/*
 * Global Configuration State:
 * These externals provide access to shared configuration state
 * used across multiple components, particularly for logging.
 */
extern const PriorityLevel DEFAULT_PRIORITY_LEVELS[NUM_PRIORITY_LEVELS];
extern int MAX_PRIORITY_LABEL_WIDTH;      // For log message formatting
extern int MAX_SUBSYSTEM_LABEL_WIDTH;     // For log message alignment

/*
 * Configuration Management Functions:
 * Core functions for loading, creating, and accessing configuration.
 * Each function includes robust error handling and logging.
 */

// Loads and validates configuration from JSON file
AppConfig* load_config(const char* config_path);

// Determines the absolute path of the server executable
char* get_executable_path();

// Utility functions for file operations
long get_file_size(const char* filename);
char* get_file_modification_time(const char* filename);

// Creates a new configuration file with safe defaults
void create_default_config(const char* config_path);

// Logging support functions
const char* get_priority_label(int priority);
void calculate_max_priority_label_width();

#endif // CONFIGURATION_H
