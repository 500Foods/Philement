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
#include "../mdns/mdns_server.h"

/*
 * System-wide Constants:
 * - VERSION: Used for compatibility checking and service advertisement
 * - Defaults: Safe fallback values when config items are missing
 * - Limits: System constraints and safety boundaries
 */
// Allow VERSION to be defined by compiler flags (e.g., -DVERSION=\"1.0.0\")
#ifndef VERSION
#define VERSION "0.1.0"
#endif
#define DEFAULT_SERVER_NAME "Philement/hydrogen"
#define DEFAULT_LOG_FILE "/var/log/hydrogen.log"
#define DEFAULT_WEB_PORT 5000              // Standard HTTP port range
#define DEFAULT_WEBSOCKET_PORT 5001        // Separate from HTTP for clean separation
#define DEFAULT_UPLOAD_PATH "/api/upload"  // REST-style API path
#define DEFAULT_UPLOAD_DIR "/tmp/hydrogen_uploads"  // Secure temp directory
#define DEFAULT_MAX_UPLOAD_SIZE (2ULL * 1024 * 1024 * 1024)  // 2GB limit for safety
#define NUM_PRIORITY_LEVELS 7              // ALL, INFO, WARN, DEBUG, ERROR, CRITICAL, NONE

/* System Resource Defaults */
#define DEFAULT_MAX_QUEUE_BLOCKS 128
#define DEFAULT_QUEUE_HASH_SIZE 256
#define DEFAULT_QUEUE_CAPACITY 1024

/* Message and Log Buffer Defaults */
#define DEFAULT_LOG_BUFFER_SIZE 256
#define DEFAULT_MESSAGE_BUFFER_SIZE 1024
#define DEFAULT_MAX_LOG_MESSAGE_SIZE 2048
#define DEFAULT_LINE_BUFFER_SIZE 512
#define DEFAULT_POST_PROCESSOR_BUFFER_SIZE 8192  // Buffer size for MHD post processor
#define DEFAULT_JSON_MESSAGE_SIZE 2048
#define DEFAULT_LOG_ENTRY_SIZE 1024
#define DEFAULT_FD_TYPE_SIZE 32
#define DEFAULT_FD_DESCRIPTION_SIZE 256

/* Network Defaults */
#define DEFAULT_MAX_INTERFACES 16
#define DEFAULT_MAX_IPS_PER_INTERFACE 8
#define DEFAULT_MAX_INTERFACE_NAME_LENGTH 16
#define DEFAULT_MAX_IP_ADDRESS_LENGTH 46

/* System Monitoring Defaults */
#define DEFAULT_STATUS_UPDATE_MS 1000
#define DEFAULT_RESOURCE_CHECK_MS 5000
#define DEFAULT_METRICS_UPDATE_MS 2000
#define DEFAULT_MEMORY_WARNING_PERCENT 90
#define DEFAULT_DISK_WARNING_PERCENT 85
#define DEFAULT_LOAD_WARNING 5.0

/* Print Queue Defaults */
#define DEFAULT_SHUTDOWN_WAIT_MS 500
#define DEFAULT_JOB_PROCESSING_TIMEOUT_MS 1000

/* Printer Motion Defaults */
#define DEFAULT_MAX_LAYERS 10000
#define DEFAULT_ACCELERATION 1000.0
#define DEFAULT_Z_ACCELERATION 250.0
#define DEFAULT_E_ACCELERATION 2000.0
#define DEFAULT_MAX_SPEED_XY 5000.0
#define DEFAULT_MAX_SPEED_TRAVEL 5000.0
#define DEFAULT_MAX_SPEED_Z 10.0
#define DEFAULT_Z_VALUES_CHUNK 100

/* Thread Management Defaults */
#define DEFAULT_THREAD_STARTUP_DELAY_US 10000
#define DEFAULT_THREAD_RETRY_DELAY_US 1000

/*
 * Logging Priority System:
 * Maps numeric priority levels to human-readable labels for consistent
 * log formatting and filtering across all server components.
 * 
 * Level Values:
 * 0 = ALL (log everything)
 * 1 = INFO
 * 2 = WARN
 * 3 = DEBUG
 * 4 = ERROR
 * 5 = CRITICAL
 * 6 = NONE (log nothing)
 */
typedef struct {
    int value;          // Numeric priority for comparison
    const char* label;  // Human-readable label for display
} PriorityLevel;

// These are just defaults, actual values come from JSON config
#define LOG_LEVEL_ALL      0
#define LOG_LEVEL_INFO     1
#define LOG_LEVEL_WARN     2
#define LOG_LEVEL_DEBUG    3
#define LOG_LEVEL_ERROR    4
#define LOG_LEVEL_CRITICAL 5
#define LOG_LEVEL_NONE     6

/*
 * Web Server Configuration:
 * Controls the HTTP server component that handles REST API requests,
 * file uploads, and static content serving. Includes safety limits
 * and path configurations.
 */
/* Web Server Thread Pool and Connection Defaults */
#define DEFAULT_THREAD_POOL_SIZE 4        // Number of worker threads
#define DEFAULT_MAX_CONNECTIONS 100       // Maximum total connections
#define DEFAULT_MAX_CONNECTIONS_PER_IP 10 // Maximum connections per IP
#define DEFAULT_CONNECTION_TIMEOUT 30     // Connection timeout in seconds

typedef struct {
    int enabled;            // Runtime toggle for web server
    int enable_ipv6;        // IPv6 support toggle
    int port;              // HTTP service port
    char *web_root;        // Static content directory
    char *upload_path;     // URL path for file uploads
    char *upload_dir;      // Storage location for uploads
    size_t max_upload_size;// Upload size limit for DoS prevention
    char *api_prefix;      // URL prefix for API endpoints (e.g., "/api")
    struct {
        int enabled;       // Enable/disable Swagger UI
        char *prefix;      // URL prefix for Swagger UI (e.g., "/docs")
        int payload_available; // Set to 1 if payload is found in executable
    } swagger;
    
    /* Thread Pool and Connection Settings */
    int thread_pool_size;     // Number of worker threads (default: DEFAULT_THREAD_POOL_SIZE)
    int max_connections;      // Maximum total connections (default: DEFAULT_MAX_CONNECTIONS)
    int max_connections_per_ip; // Maximum connections per IP (default: DEFAULT_MAX_CONNECTIONS_PER_IP)
    int connection_timeout;   // Connection timeout in seconds (default: DEFAULT_CONNECTION_TIMEOUT)
} WebConfig;

/*
 * Logging Configuration:
 * Comprehensive logging system with support for multiple destinations
 * and per-subsystem log levels.
 */
typedef struct {
    struct {
        int ThreadMgmt;      // Thread management logging
        int Shutdown;        // Shutdown process logging
        int mDNSServer;     // mDNS Server service logging
        int WebServer;      // Web server logging
        int WebSocket;      // WebSocket server logging
        int PrintQueue;     // Print queue logging
        int LogQueueManager;// Log queue manager logging
    } Subsystems;
    int DefaultLevel;      // Default logging level
    int Enabled;            // Whether this destination is active
    union {
        struct {
            char *Path;      // Log file path
        } File;
        struct {
            char *ConnectionString;  // Database connection
        } Database;
    } Config;
} LoggingDestination;

typedef struct {
    struct {
        int value;
        const char *name;
    } *Levels;                  // Array of level definitions
    size_t LevelCount;          // Number of defined levels
    LoggingDestination Console;  // Console output configuration
    LoggingDestination File;     // File output configuration
    LoggingDestination Database; // Database output configuration
} LoggingConfig;

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
    mdns_server_service_t *services; // Array of advertised services
    size_t num_services;   // Number of advertised services
} mDNSServerConfig;

/*
 * Print Queue Configuration:
 * Controls the print job management system, including job
 * scheduling, status tracking, and error handling.
 */
typedef struct {
    size_t max_queue_blocks;
    size_t queue_hash_size;
    size_t default_capacity;
    size_t message_buffer_size;
    size_t max_log_message_size;
    size_t line_buffer_size;
    size_t log_buffer_size;      // Size for general log buffers
    size_t json_message_size;    // Size for JSON message buffers
    size_t log_entry_size;       // Size for log entry buffers
    size_t fd_type_size;         // Size for file descriptor type strings
    size_t fd_description_size;  // Size for file descriptor descriptions
    size_t post_processor_buffer_size;  // Size for MHD post processor buffer
} SystemResourcesConfig;

typedef struct {
    size_t max_interfaces;
    size_t max_ips_per_interface;
    size_t max_interface_name_length;
    size_t max_ip_address_length;
    int start_port;
    int end_port;
    int* reserved_ports;
    size_t reserved_ports_count;
} NetworkConfig;

typedef struct {
    size_t status_update_ms;
    size_t resource_check_ms;
    size_t metrics_update_ms;
    int memory_warning_percent;
    int disk_warning_percent;
    float load_warning;
} SystemMonitoringConfig;

typedef struct {
    size_t max_layers;          // Maximum number of layers
    double acceleration;        // Default acceleration (mm/s²)
    double z_acceleration;      // Z-axis acceleration (mm/s²)
    double e_acceleration;      // Extruder acceleration (mm/s²)
    double max_speed_xy;       // Maximum XY speed (mm/s)
    double max_speed_travel;   // Maximum travel speed (mm/s)
    double max_speed_z;        // Maximum Z speed (mm/s)
    size_t z_values_chunk;     // Size of Z-values array chunks
} PrinterMotionConfig;

typedef struct {
    int enabled;            // Runtime toggle for print queue
    struct {
        int default_priority;
        int emergency_priority;
        int maintenance_priority;
        int system_priority;
    } priorities;
    struct {
        size_t shutdown_wait_ms;
        size_t job_processing_timeout_ms;
    } timeouts;
    struct {
        size_t job_message_size;
        size_t status_message_size;
    } buffers;
} PrintQueueConfig;

/*
 * Main Application Configuration:
 * Top-level configuration structure that aggregates all component
 * configurations. This represents the complete runtime configuration
 * of the Hydrogen server.
 */
/*
 * OIDC Configuration:
 * Settings for the OpenID Connect Identity Provider functionality.
 * Includes endpoint paths, token lifetimes, security policies, and key management.
 */
typedef struct {
    char *authorization;    // Authorization endpoint path
    char *token;           // Token endpoint path
    char *userinfo;        // Userinfo endpoint path
    char *jwks;            // JSON Web Key Set endpoint path
    char *introspection;   // Token introspection endpoint path
    char *revocation;      // Token revocation endpoint path
    char *registration;    // Client registration endpoint path
} OIDCEndpointsConfig;

typedef struct {
    int rotation_interval_days;  // Number of days before key rotation
    char *storage_path;         // Path to store key files
    int encryption_enabled;     // Whether to encrypt stored keys
} OIDCKeysConfig;

typedef struct {
    int access_token_lifetime;   // Access token lifetime in seconds
    int refresh_token_lifetime;  // Refresh token lifetime in seconds
    int id_token_lifetime;       // ID token lifetime in seconds
} OIDCTokensConfig;

typedef struct {
    int require_pkce;            // Require PKCE for authorization code flow
    int allow_implicit_flow;     // Allow implicit flow (less secure)
    int allow_client_credentials; // Allow client credentials flow
    int require_consent;         // Require user consent screen
} OIDCSecurityConfig;

typedef struct {
    int enabled;                // Runtime toggle for OIDC service
    char *issuer;              // Issuer URI for this IdP (e.g., https://example.com)
    OIDCEndpointsConfig endpoints; // OIDC endpoint configurations
    OIDCKeysConfig keys;          // Key management settings
    OIDCTokensConfig tokens;      // Token lifetime settings
    OIDCSecurityConfig security;  // Security policy settings
} OIDCConfig;

/*
 * API Configuration:
 * Settings for REST API endpoints, including authentication and common behaviors.
 */
typedef struct {
    char *jwt_secret;        // Secret key for signing and validating API JWTs
} APIConfig;

typedef struct {
    char *server_name;      // Server identification
    char *executable_path;  // Binary location for resource loading
    char *log_file_path;   // Central log file location
    WebConfig web;         // HTTP server settings
    WebSocketConfig websocket; // WebSocket server settings
    mDNSServerConfig mdns_server;       // Service discovery settings
    PrintQueueConfig print_queue;     // Print management settings
    LoggingConfig Logging;            // Logging configuration
    SystemResourcesConfig resources;   // System resource settings
    NetworkConfig network;            // Network interface settings
    SystemMonitoringConfig monitoring; // System monitoring settings
    PrinterMotionConfig motion;       // Printer motion settings
    OIDCConfig oidc;       // OIDC service configuration
    APIConfig api;         // API service configuration
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
