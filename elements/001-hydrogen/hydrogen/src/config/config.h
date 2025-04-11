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

// Support for Key/Value values of different types including env vars.
#include "config_utils.h"
#include "types/config_string.h"
#include "types/config_bool.h"
#include "types/config_int.h"
#include "types/config_size.h"
#include "types/config_double.h"
#include "types/config_fd.h"

// Main application configuration structure
struct AppConfig {
    ServerConfig server;           // A. Server configuration
    NetworkConfig network;         // B. Network configuration
    DatabaseConfig databases;      // C. Database configuration
    LoggingConfig logging;         // D. Logging configuration
    WebServerConfig web;           // E. WebServer configuration
    APIConfig api;                 // F. API configuration
    SwaggerConfig* swagger;        // G. Swagger configuration
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

#endif /* CONFIG_H */