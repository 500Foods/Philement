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

// Forward declarations for all configuration types
#include "config_forward.h"

// Subsystem configurations with struct definitions
#include "webserver/config_webserver.h"
#include "websocket/config_websocket.h"
#include "network/config_network.h"
#include "monitor/config_monitoring.h"
#include "print/config_print_queue.h"
#include "oidc/config_oidc.h"
#include "resources/config_resources.h"
#include "mdns/config_mdns.h"
#include "logging/config_logging.h"
#include "api/config_api.h"
#include "notify/config_notify.h"
#include "print/config_motion.h"

// Type-specific handling
#include "env/config_env.h"
#include "types/config_string.h"
#include "types/config_bool.h"
#include "types/config_int.h"
#include "types/config_size.h"
#include "types/config_double.h"
#include "types/config_fd.h"

// Core application defaults
#define DEFAULT_SERVER_NAME "Philement/hydrogen"
#define DEFAULT_CONFIG_FILE "/etc/hydrogen/hydrogen.json"
#define DEFAULT_STARTUP_DELAY 5

// Server configuration structure
typedef struct ServerConfig {
    char* server_name;      // Server identification
    char* payload_key;      // Key for payload encryption
    char* config_file;      // Main configuration file path
    char* exec_file;        // Executable file path
    char* log_file;        // Log file path
    int startup_delay;     // Delay before starting services (seconds)
} ServerConfig;

// Main application configuration structure
struct AppConfig {
    ServerConfig server;
    
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
    NotifyConfig notify;
    LoggingConfig logging;
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
 * Helper function to handle environment variable logging
 * This function is exported for use by config_env.c
 * 
 * @param key_name The configuration key name
 * @param var_name The environment variable name
 * @param env_value The value from the environment variable
 * @param default_value The default value if not set
 * @param is_sensitive Whether this contains sensitive information
 */
void log_config_env_value(const char* key_name, const char* var_name, const char* env_value, 
                       const char* default_value, bool is_sensitive);

#endif /* CONFIG_H */