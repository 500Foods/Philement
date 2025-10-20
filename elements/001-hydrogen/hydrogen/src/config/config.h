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
 * B. Network (subsystem)
 * C. Database (subsystem)
 * D. Logging (subsystem)
 * E. WebServer (subsystem)
 * F. API (subsystem)
 * G. Swagger (subsystem)
 * H. WebSocket (subsystem)
 * I. Terminal (subsystem)
 * J. mDNS Server(subsystem)
 * K. mDNS Client (subsystem)
 * L. Mail Relay (subsystem)
 * M. Print (subsystem)
 * N. Resources (subsystem)
 * O. OIDC (subsystem)
 * P. Notify (subsystem)
 * 
 * NOTE: threads, registry, and payload are subsystems that don't 
 * have their own sections in the config system because they are
 * mostly internal or, like threads, are found in the individual
 * sections where appropriate.
 */

#ifndef CONFIG_H
#define CONFIG_H

// Core system headers
#include <sys/types.h>
#include <stddef.h>
#include <stdbool.h>


// File operation utilities
bool is_file_readable(const char* path);

// Forward declarations for all configuration types
#include "config_forward.h"

// Subsystem configurations with struct definitions
#include "config_defaults.h"
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
 * Helper function to clean up all resources in AppConfig
 *
 * @param config Pointer to AppConfig structure to cleanup
 */
void clean_app_config(AppConfig* config);

/*
 * Count UTF-8 characters in a string
 *
 * @param str String to count characters in
 * @return Number of UTF-8 characters
 */
size_t utf8_char_count(const char* str);

/*
 * Truncate string to N UTF-8 characters
 *
 * @param str String to truncate (modified in place)
 * @param max_chars Maximum number of UTF-8 characters
 */
void utf8_truncate(char* str, size_t max_chars);

/*
 * Format a section header with proper UTF-8 handling
 *
 * @param buffer Output buffer
 * @param size Size of output buffer
 * @param letter Section letter
 * @param name Section name
 */
void format_section_header(char* buffer, size_t size, const char* letter, const char* name);

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
