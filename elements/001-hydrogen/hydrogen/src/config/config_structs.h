/*
 * Configuration structures
 * 
 * This module defines all configuration structures used by the system.
 * Structures are organized by subsystem for clarity and maintainability.
 */

#ifndef CONFIG_STRUCTS_H
#define CONFIG_STRUCTS_H

#include <jansson.h>
#include "../mdns/mdns_server.h"  // For mdns_server_service_t
#include "config_forward.h"       // For AppConfig forward declaration

// Queue system defaults
#define DEFAULT_MAX_QUEUE_BLOCKS 1024
#define DEFAULT_QUEUE_HASH_SIZE 256
#define DEFAULT_QUEUE_CAPACITY 1000
#define DEFAULT_MESSAGE_BUFFER_SIZE 4096
#define DEFAULT_MAX_LOG_MESSAGE_SIZE 1024
#define DEFAULT_LINE_BUFFER_SIZE 256
#define DEFAULT_POST_PROCESSOR_BUFFER_SIZE (64 * 1024)
#define DEFAULT_LOG_BUFFER_SIZE (32 * 1024)
#define DEFAULT_JSON_MESSAGE_SIZE 8192
#define DEFAULT_LOG_ENTRY_SIZE 512

// Web server configuration
typedef struct {
    int enabled;
    int enable_ipv6;
    int port;
    char* web_root;
    char* upload_path;
    char* upload_dir;
    size_t max_upload_size;
    char* api_prefix;
    
    // Thread pool and connection settings
    int thread_pool_size;
    int max_connections;
    int max_connections_per_ip;
    int connection_timeout;

    // Swagger UI configuration
        struct {
            int enabled;
            char* prefix;
            int payload_available;  // Track if swagger payload was loaded
            
            struct {
            char* title;
            char* description;
            char* version;
            
            struct {
                char* name;
                char* email;
                char* url;
            } contact;
            
            struct {
                char* name;
                char* url;
            } license;
        } metadata;
        
        struct {
            int try_it_enabled;
            int always_expanded;
            int display_operation_id;
            int default_models_expand_depth;
            int default_model_expand_depth;
            int show_extensions;
            int show_common_extensions;
            char* doc_expansion;
            char* syntax_highlight_theme;
        } ui_options;
    } swagger;
} WebServerConfig;

// WebSocket configuration
typedef struct {
    int enabled;
    int enable_ipv6;
    int port;
    char* key;
    char* protocol;
    size_t max_message_size;
    int exit_wait_seconds;
} WebSocketConfig;

// mDNS server configuration
typedef struct {
    int enabled;
    int enable_ipv6;
    char* device_id;
    char* friendly_name;
    char* model;
    char* manufacturer;
    char* version;
    mdns_server_service_t* services;
    size_t num_services;
} MDNSServerConfig;

// System resources configuration
typedef struct {
    size_t max_queue_blocks;
    size_t queue_hash_size;
    size_t default_capacity;
    size_t message_buffer_size;
    size_t max_log_message_size;
    size_t line_buffer_size;
    size_t post_processor_buffer_size;
    size_t log_buffer_size;
    size_t json_message_size;
    size_t log_entry_size;
} ResourceConfig;

// Network configuration
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

// System monitoring configuration
typedef struct {
    size_t status_update_ms;
    size_t resource_check_ms;
    size_t metrics_update_ms;
    int memory_warning_percent;
    int disk_warning_percent;
    double load_warning;
} MonitoringConfig;

// Motion configuration
typedef struct {
    size_t max_layers;
    double acceleration;
    double z_acceleration;
    double e_acceleration;
    double max_speed_xy;
    double max_speed_travel;
    double max_speed_z;
    size_t z_values_chunk;
} MotionConfig;

// Print queue configuration
typedef struct {
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

    int enabled;
} PrintQueueConfig;

// OIDC configuration
typedef struct {
    int enabled;
    char* issuer;
    
    struct {
        char* authorization;
        char* token;
        char* userinfo;
        char* jwks;
        char* introspection;
        char* revocation;
        char* registration;
    } endpoints;
    
    struct {
        int rotation_interval_days;
        char* storage_path;
        int encryption_enabled;
    } keys;
    
    struct {
        int access_token_lifetime;
        int refresh_token_lifetime;
        int id_token_lifetime;
    } tokens;
    
    struct {
        int require_pkce;
        int allow_implicit_flow;
        int allow_client_credentials;
        int require_consent;
    } security;
} OIDCConfig;

// API configuration
typedef struct {
    char* jwt_secret;
} APIConfig;


// Base structure for all logging destinations
typedef struct {
    int Enabled;
    int DefaultLevel;
    struct {
        int ThreadMgmt;
        int Shutdown;
        int mDNSServer;
        int WebServer;
        int WebSocket;
        int PrintQueue;
        int LogQueueManager;
    } Subsystems;
} LoggingDestination;

typedef struct {
    LoggingDestination base;
} LoggingConsoleConfig;

typedef struct {
    LoggingDestination base;
    char* FilePath;
    size_t MaxFileSize;
    int RotateFiles;
} LoggingFileConfig;

typedef struct {
    LoggingDestination base;
    char* ConnectionString;
    char* TableName;
    int BatchSize;
    int FlushIntervalMs;
} LoggingDatabaseConfig;

typedef struct {
    size_t LevelCount;
    struct {
        int value;
        const char* name;
    }* Levels;

    LoggingConsoleConfig Console;
    LoggingFileConfig File;
    LoggingDatabaseConfig Database;
} LoggingConfig;

// Main application configuration
struct AppConfig {
    char* config_file;
    char* executable_path;
    char* server_name;
    char* payload_key;
    char* log_file_path;
    
    WebServerConfig web;
    WebSocketConfig websocket;
    MDNSServerConfig mdns_server;
    ResourceConfig resources;
    NetworkConfig network;
    MonitoringConfig monitoring;
    MotionConfig motion;
    PrintQueueConfig print_queue;
    OIDCConfig oidc;
    APIConfig api;
    LoggingConfig Logging;
};

#endif /* CONFIG_STRUCTS_H */