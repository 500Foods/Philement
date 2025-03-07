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

// Version information
#ifndef VERSION
#define VERSION "0.1.0"
#endif

// Forward declarations for all configuration types
#include "config_forward.h"

// Subsystem configurations with struct definitions
#include "config_webserver.h"
#include "config_websocket.h"
#include "config_network.h"
#include "config_monitoring.h"
#include "config_print_queue.h"
#include "config_oidc.h"
#include "config_resources.h"
#include "config_mdns.h"
#include "config_logging.h"
#include "config_api.h"
#include "config_motion.h"

// Type-specific handling
#include "config_env.h"
#include "config_string.h"
#include "config_bool.h"
#include "config_int.h"
#include "config_size.h"
#include "config_double.h"
#include "config_fd.h"

// Core application defaults
#define DEFAULT_SERVER_NAME "Philement/hydrogen"
#define DEFAULT_CONFIG_FILE "/etc/hydrogen/hydrogen.json"
#define DEFAULT_STARTUP_DELAY 5

// Server configuration structure
typedef struct ServerConfig {
    char* server_name;
    char* payload_key;
    char* config_file;
    char* exec_file;
    char* log_file;
    int startup_delay;
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

#endif /* CONFIG_H */