/*
 * Configuration management system with robust fallback handling
 * 
 * If available, this is intended to read in a JSON configuration with
 * appropriate sections. If sections are missing, keys are missing, or
 * if no configuration is found at startup, defaults will be used.
 * 
 * Environment variables can also be used to supply key values and some
 * are used even when no configuration JSON is available, like the tokens
 * for the payload and the Acuranzo database connection information.
 * 
 * The log should make it *abundantly* clear what is going on, so that
 * issues with the configuration, or lack of configuration, are obvious.
 * 
 * The top-level JSON sections we're expecting are the following, and
 * largely match the order of processing of the subsystems they apply to.
 * 
 * System Monitoring
 * A. Server
 * B. Network
 * C. Database
 * D. Logging
 * E. WebServer
 * F. API
 * G. Swagger
 * H. WebSocket
 * I. Terminal
 * J. mDNS Server
 * K. mDNS Client
 * L. Mail Relay
 * M. Print
 * N. Resources
 * O. OIDC
 * P. Notify
 */

#ifndef CONFIG_H
#define CONFIG_H

// Core system headers
#include <sys/types.h>
#include <stddef.h>
#include <stdbool.h>

// Version information
#ifndef VERSION
#define VERSION "0.1.0"
#endif

/*
 * Buffer and Layer Configuration Constants
 * These constants are used throughout the system for
 * consistent buffer allocation and layer handling.
 */

// Default buffer sizes
#define DEFAULT_LINE_BUFFER_SIZE 4096           // 4KB for line buffers
#define DEFAULT_LOG_BUFFER_SIZE 8192            // 8KB for log messages
#define DEFAULT_POST_PROCESSOR_BUFFER_SIZE 8192 // 8KB for post processing
#define DEFAULT_COMMAND_BUFFER_SIZE 4096        // 4KB for commands
#define DEFAULT_RESPONSE_BUFFER_SIZE 16384      // 16KB for responses

// Layer limits
#define DEFAULT_MAX_LAYERS 1000                 // Maximum number of layers
#define MIN_LAYERS 1                            // Minimum number of layers
#define MAX_LAYERS 10000                        // Absolute maximum layers

/*
 * Queue Configuration Constants
 * These constants are used by both the configuration system
 * and the queue utilities.
 */

// Queue size limits
#define DEFAULT_MAX_QUEUE_SIZE 10000            // 10K items
#define DEFAULT_MAX_QUEUE_MEMORY_MB 256         // 256MB default
#define DEFAULT_MAX_QUEUE_BLOCKS 1024           // 1K blocks
#define DEFAULT_QUEUE_TIMEOUT_MS 30000          // 30 seconds

// Queue validation limits
#define MIN_QUEUE_SIZE 10
#define MAX_QUEUE_SIZE 1000000
#define MIN_QUEUE_MEMORY_MB 64
#define MAX_QUEUE_MEMORY_MB 16384
#define MIN_QUEUE_BLOCKS 64
#define MAX_QUEUE_BLOCKS 16384
#define MIN_QUEUE_TIMEOUT_MS 1000
#define MAX_QUEUE_TIMEOUT_MS 300000


// File operation utilities
bool is_file_readable(const char* path);

// Forward declarations for all configuration types
#include "config_forward.h"

// Subsystem configurations with struct definitions
#include "config_server.h"               // A. Server
#include "config_network.h"              // B. Network
#include "config_databases.h"            // C. Database
#include "config_logging.h"              // D. Logging
#include "config_webserver.h"            // E. WebServer
#include "config_api.h"                  // F. API
#include "config_swagger.h"              // G. Swagger
#include "config_websocket.h"            // H. WebSocket
#include "config_terminal.h"             // I. Terminal
#include "config_mdns_server.h"          // J. mDNS Server
#include "config_mdns_client.h"          // K. mDNS Client
#include "config_mail_relay.h"           // L. Mail Relay
#include "config_print.h"                // M. Print
#include "config_resources.h"            // N. Resources
#include "config_oidc.h"                 // O. OIDC
#include "config_notify.h"               // P. Notify

// Support for configuration value handling and type conversion
#include "config_utils.h"  // Includes string handling and file operations

// Main application configuration structure
struct AppConfig {
    ServerConfig server;           // A. Server configuration
    NetworkConfig network;         // B. Network configuration
    DatabaseConfig databases;      // C. Database configuration
    LoggingConfig logging;         // D. Logging configuration
    WebServerConfig webserver;     // E. WebServer configuration
    APIConfig api;                 // F. API configuration
    SwaggerConfig swagger;         // G. Swagger configuration
    WebSocketConfig websocket;     // H. WebSocket configuration
    TerminalConfig terminal;       // I. Terminal configuration
    MDNSServerConfig mdns_server;  // J. mDNS Server configuration
    MDNSClientConfig mdns_client;  // K. mDNS Client configuration
    MailRelayConfig mail_relay;    // L. Mail Relay configuration
    PrintConfig print;            // M. Print configuration
    ResourceConfig resources;      // N. Resources configuration
    OIDCConfig oidc;               // O. OIDC configuration
    NotifyConfig notify;           // P. Notify configuration
};

/*
 * Get the current application configuration
 * 
 * Returns a pointer to the current application configuration.
 * This configuration is loaded by load_config() and stored in a static variable.
 * The returned pointer should not be modified by the caller.
 */
const AppConfig* get_app_config(void);

/*
 * Load and validate configuration with comprehensive error handling
 * 
 * Loads configuration from the specified file, applying:
 * - Type validation
 * - Range checking
 * - Default value fallbacks
 * - Environment variable substitution
 * 
 * @param config_path Path to the configuration file
 * @return Pointer to loaded configuration or NULL on error
 */
AppConfig* load_config(const char* config_path);

/*
 * Generate default configuration with secure baseline
 * 
 * Creates a new configuration file with secure defaults:
 * - Conservative file permissions
 * - Secure network settings
 * - Resource limits
 * - Standard service ports
 * 
 * @param config_path Path where configuration should be created
 */
void create_default_config(const char* config_path);

/*
 * Perform final cleanup of app configuration during shutdown
 * This function should be called during the shutdown sequence
 * to prevent memory leaks.
 */
void cleanup_application_config(void);

/*
 * Helper function to handle environment variable logging
 * This function is exported for use by config_utils.c
 * 
 * @param key_name The configuration key name
 * @param var_name The environment variable name
 * @param env_value The value from the environment variable
 * @param default_value The default value if not set
 * @param is_sensitive Whether this contains sensitive information
 */
void log_config_env_value(
    const char* key_name, 
    const char* var_name, 
    const char* env_value, 
    const char* default_value, 
    bool is_sensitive
);

/*
 * Debug function to dump the current state of AppConfig
 * Shows raw configuration values loaded so far, matching JSON structure.
 * Called after each section load to verify configuration state.
 * 
 * @param config Pointer to the AppConfig structure
 * @param section Current section being processed (or NULL for all)
 */
void dumpAppConfig(const AppConfig* config, const char* section);

/*
 * Get the current configuration as formatted text
 * 
 * Similar to dumpAppConfig but writes to a provided buffer instead of logs.
 * The caller is responsible for freeing the returned string.
 * 
 * @param config Pointer to the AppConfig structure
 * @param section Current section to format (or NULL for all)
 * @return Newly allocated string containing the formatted config, or NULL on error
 */
char* getAppConfigText(const AppConfig* config, const char* section);

#endif /* CONFIG_H */