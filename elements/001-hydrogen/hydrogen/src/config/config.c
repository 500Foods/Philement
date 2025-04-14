/*
 * Configuration Management System
 * -----------------------------
 * Manages application configuration through a hierarchical system with fallbacks:
 * 1. JSON config file (optional, searched in standard locations)
 * 2. Environment variables (can override JSON values)
 * 3. Built-in defaults (secure baseline when nothing else specified)
 *
 * Configuration Sections (A-P):
 * System Monitoring
 * A. Server        F. API           K. mDNS Client    P. Notify
 * B. Network       G. Swagger       L. Mail Relay
 * C. Database      H. WebSocket     M. Print
 * D. Logging       I. Terminal      N. Resources
 * E. WebServer     J. mDNS Server   O. OIDC
 *
 * Core Principles:
 * - AppConfig structure holds ALL runtime configuration
 * - Sections processed in A-P order to match subsystem startup
 * - Environment variables use ${env.NAME} syntax in JSON or defaults
 * - Missing values fall back to secure defaults
 * - Config reloaded on restart to pick up changes
 *
 * Security & Logging:
 * - Sensitive values (tokens, passwords) partially masked in logs
 * - Default values marked with asterisk (*) in logs
 * - Missing required env vars logged as ERROR
 * - Type mismatches (e.g., invalid port) logged as ERROR
 * - Structured logging format for consistency across sections
 * - Indentation with hyphens to match JSON depth
 * - Logging should look like Key: Value normally
 * -   EG: -- Enabled: true
 * - If an env var is used, then Key {env var}: Value 
 * -   EG: -- Port {PORT}: 8080
 * - And if a secret is envolved, then Key {env var}: Secre..... (first 5 chars only)
 * -   EG: -- JWTSecret {JWT_SECRET}: abcde...
 * - We shouldn't ever see output in the log showing ${env.VAR} 
 *-
 * Implementation Notes:
 * - json_SECTION.c files handle JSON parsing (one line per key)
 * - config_SECTION.c files manage AppConfig population and validation
 * - Each section handles: missing files, env vars, defaults, validation
 * - Strict type checking and range validation on all values
 */

// Core system headers
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Project headers
#include "config.h"
#include "config_utils.h"
#include "types/config_string.h"
#include "types/config_bool.h"
#include "types/config_int.h"
#include "types/config_size.h"
#include "types/config_double.h"
#include "files/config_filesystem.h"
#include "files/config_file_utils.h"
#include "config_priority.h"

// Configuration system
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

// Core system headers
#include "../logging/logging.h"
#include "../utils/utils.h"

// JSON loading functions - sections being migrated from json_* to config_*

// Global static configuration instance
static AppConfig *app_config = NULL;

// Forward declaration for cleanup function
static void clean_app_config(AppConfig* config);

// Standard system paths to check for configuration
static const char* const CONFIG_PATHS[] = {
    "hydrogen.json",
    "/etc/hydrogen/hydrogen.json",
    "/usr/local/etc/hydrogen/hydrogen.json"
};
static const int NUM_CONFIG_PATHS = sizeof(CONFIG_PATHS) / sizeof(CONFIG_PATHS[0]);

// Function forward declarations
const AppConfig* get_app_config(void);
AppConfig* load_config(const char* cmdline_path);

// Function declarations for configuration loading
bool load_server_config(json_t* root, AppConfig* config, const char* config_path);
bool load_network_config(json_t* root, AppConfig* config);
bool load_database_config(json_t* root, AppConfig* config);
bool load_logging_config(json_t* root, AppConfig* config);
bool load_webserver_config(json_t* root, AppConfig* config);
bool load_api_config(json_t* root, AppConfig* config);
bool load_swagger_config(json_t* root, AppConfig* config);
bool load_websocket_config(json_t* root, AppConfig* config);
bool load_terminal_config(json_t* root, AppConfig* config);
bool load_mdns_server_config(json_t* root, AppConfig* config);
bool load_mdns_client_config(json_t* root, AppConfig* config);
bool load_mailrelay_config(json_t* root, AppConfig* config);
bool load_print_config(json_t* root, AppConfig* config);
bool load_resources_config(json_t* root, AppConfig* config);
bool load_oidc_config(json_t* root, AppConfig* config);
bool load_notify_config(json_t* root, AppConfig* config);

/*
 * Load and validate configuration with comprehensive error handling
 * 
 * This function loads the configuration from a file specified by the command line,
 * environment variable, or standard locations. It validates the configuration and
 * returns a pointer to the configuration structure.
 * 
 * @param cmdline_path The path to the configuration file specified on the command line
 * @return Pointer to the loaded configuration, or NULL on error
 */
AppConfig* load_config(const char* cmdline_path) {
    log_this("Config", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Config", "CONFIGURATION", LOG_LEVEL_STATE);

    // Free previous configuration if it exists
    if (app_config) {
        clean_app_config(app_config);
        free(app_config);
    }

    json_t* root = NULL;
    json_error_t error;
    const char* final_path = NULL;
    bool explicit_config = false;

    // First check HYDROGEN_CONFIG environment variable
    const char* env_path = getenv("HYDROGEN_CONFIG");
    if (env_path) {
        explicit_config = true;
        if (!is_file_readable(env_path)) {
            log_this("Config", "― Env config file not found: %s", LOG_LEVEL_ERROR, env_path);
            return NULL;
        }
        root = json_load_file(env_path, 0, &error);
        if (!root) {
            log_this("Config", "― Failed to load env config: %s (line %d, column %d)",
                     LOG_LEVEL_ERROR, error.text, error.line, error.column);
            return NULL;
        }
        final_path = env_path;
        log_this("Config", "― Using env config: %s", LOG_LEVEL_STATE, env_path);
    }

    // Then try command line path if provided
    if (!root && cmdline_path) {
        explicit_config = true;
        if (!is_file_readable(cmdline_path)) {
            log_this("Config", "― Param config file not found: %s", LOG_LEVEL_ERROR, cmdline_path);
            return NULL;
        }
        root = json_load_file(cmdline_path, 0, &error);
        if (!root) {
            dprintf(STDERR_FILENO, "― JSON parse error at line %d, column %d: %s\n", error.line, error.column, error.text);
            log_this("Config", "― Failed to load param config: %s (line %d, column %d)",
                     LOG_LEVEL_ERROR, error.text, error.line, error.column);
            return NULL;
        }
        final_path = cmdline_path;
        log_this("Config", "― Using param config: %s", LOG_LEVEL_STATE, cmdline_path);
    }

    // If no explicit config was provided, try standard locations
    if (!root && !explicit_config) {
        for (int i = 0; i < NUM_CONFIG_PATHS; i++) {
            if (is_file_readable(CONFIG_PATHS[i])) {
                root = json_load_file(CONFIG_PATHS[i], 0, &error);
                if (root) {
                    final_path = CONFIG_PATHS[i];
                    log_this("Config", "― Using config from: %s", LOG_LEVEL_STATE, final_path);
                    break;
                }
                // If file exists but has errors, try next location
                log_this("Config", "― Skipping %s due to parse errors", LOG_LEVEL_ALERT, CONFIG_PATHS[i]);
            }
        }
    }

    // Allocate config structure
    AppConfig* config = calloc(1, sizeof(AppConfig));
    if (!config) {
        log_this("Config", "― Failed to allocate memory for config", LOG_LEVEL_ERROR);
        if (root) json_decref(root);
        return NULL;
    }
    
    app_config = config;

    // Set up config path for use in server section
    const char* config_path = final_path;
    if (!config_path) {
        config_path = "― Missing... using defaults";
    }

    // If no config file was found, log the checked locations
    if (!root) {
        log_this("Config", "― No configuration file found, using defaults", LOG_LEVEL_ALERT);
        log_this("Config", "― Checked locations:", LOG_LEVEL_STATE);
        if (env_path) {
            log_this("Config", "――― $HYDROGEN_CONFIG: %s", LOG_LEVEL_STATE, env_path);
        }
        if (cmdline_path) {
            log_this("Config", "――― Command line path: %s", LOG_LEVEL_STATE, cmdline_path);
        }
        for (int i = 0; i < NUM_CONFIG_PATHS; i++) {
            log_this("Config", "――― %s", LOG_LEVEL_STATE, CONFIG_PATHS[i]);
        }
    }

    /*
     * Configuration loading follows the standard section order.
     * Each section must handle:
     * - Missing configuration file
     * - Missing section in config
     * - Environment variable overrides
     * - Default values
     */

    // Macro for server config (special case with extra parameter)
    #define LOAD_SERVER_CONFIG(letter, name, func, path) \
        if (!func(root, config, path)) { \
            if (root) json_decref(root); \
            return NULL; \
        }

    // Macro for standard config sections
    #define LOAD_CONFIG(letter, name, func) \
        if (!func(root, config)) { \
            if (root) json_decref(root); \
            return NULL; \
        }

    // Load configurations in A-P order
    LOAD_SERVER_CONFIG("A", "Server",       load_server_config, config_path);
    LOAD_CONFIG("B", "Network",      load_network_config);
    LOAD_CONFIG("C", "Database",     load_database_config);
    LOAD_CONFIG("D", "Logging",      load_logging_config);
    LOAD_CONFIG("E", "WebServer",    load_webserver_config);
    LOAD_CONFIG("F", "API",          load_api_config);

    dumpAppConfig(config, NULL);  // Show complete config after network

    LOAD_CONFIG("G", "Swagger",      load_swagger_config);
    LOAD_CONFIG("H", "WebSocket",    load_websocket_config);
    LOAD_CONFIG("I", "Terminal",     load_terminal_config);
    LOAD_CONFIG("J", "mDNS Server",  load_mdns_server_config);
    LOAD_CONFIG("K", "mDNS Client",  load_mdns_client_config);
    LOAD_CONFIG("L", "Mail Relay",   load_mailrelay_config);
    LOAD_CONFIG("M", "Print",        load_print_config);
    LOAD_CONFIG("N", "Resources",    load_resources_config);
    LOAD_CONFIG("O", "OIDC",         load_oidc_config);
    LOAD_CONFIG("P", "Notify",       load_notify_config);

    #undef LOAD_SERVER_CONFIG
    #undef LOAD_CONFIG

    if (root) json_decref(root);
    
    return config;
}
// Count UTF-8 characters
static size_t utf8_char_count(const char* str) {
    size_t chars = 0;
    while (*str) {
        if ((*str & 0xC0) != 0x80) { // Start byte
            chars++;
        }
        str++;
    }
    return chars;
}

// Truncate to N UTF-8 characters
static void utf8_truncate(char* str, size_t max_chars) {
    size_t chars = 0;
    char* pos = str;
    char* last_valid = str; // Track start of last complete character

    while (*pos) {
        if ((*pos & 0xC0) != 0x80) { // Start byte
            if (chars < max_chars) {
                last_valid = pos; // Update last valid character start
                chars++;
            } else {
                break;
            }
        }
        pos++;
    }
    *last_valid = '\0'; // Truncate at start of last valid character
}

static void format_section_header(char* buffer, size_t size, const char* letter, const char* name) {
    if (!buffer || !letter || !name || size <= strlen(LOG_LINE_BREAK)) {
        if (buffer) buffer[0] = '\0';
        return;
    }

    // Get LOG_LINE_BREAK's character count
    size_t target_char_count = utf8_char_count(LOG_LINE_BREAK);
    if (size <= strlen(LOG_LINE_BREAK)) {
        buffer[0] = '\0';
        return;
    }

    // Build: 3 emdashes + space + title + space + LOG_LINE_BREAK
    char temp[256] = {0};
    snprintf(temp, sizeof(temp), "――― %s. %s %s", letter, name, LOG_LINE_BREAK);

    // Uppercase title (between 3 emdashes + space and next space)
    size_t title_start = 10; // 3 * 3 + 1
    for (size_t i = title_start; temp[i] && temp[i] != ' '; i++) {
        temp[i] = toupper(temp[i]);
    }

    // Truncate to target_char_count characters
    utf8_truncate(temp, target_char_count);

    // Copy to buffer
    size_t len = strlen(temp);
    if (len >= size) {
        buffer[0] = '\0';
        return;
    }
    memcpy(buffer, temp, len + 1);
}

/*
 * Debug function to dump the current state of AppConfig
 * Shows raw configuration values loaded so far.
 */
// Maximum length for section headers
#define MAX_HEADER_LENGTH 256

void dumpAppConfig(const AppConfig* config, const char* section) {
    if (!config) {
        log_this("Config", "Cannot dump NULL config", LOG_LEVEL_TRACE);
        return;
    }

    char header[MAX_HEADER_LENGTH];

    format_section_header(header, sizeof(header), "AppConfig Dump Started", "");
    log_this("Config-Dump", "%s", LOG_LEVEL_STATE, header);

    // Server section (has implementation)
    if (!section || strcmp(section, "Server") == 0) {
        format_section_header(header, sizeof(header), "A", "Server");
        log_this("Config-Dump", "%s", LOG_LEVEL_STATE, header);
        dump_server_config(&config->server);
    }

    // Network section
    if (!section || strcmp(section, "Network") == 0) {
        format_section_header(header, sizeof(header), "B", "Network");
        log_this("Config-Dump", "%s", LOG_LEVEL_STATE, header);
        dump_network_config(&config->network);
    }

    // Databases section
    if (!section || strcmp(section, "Databases") == 0) {
        format_section_header(header, sizeof(header), "C", "Databases");
        log_this("Config-Dump", "%s", LOG_LEVEL_STATE, header);
        dump_database_config(&config->databases);
    }

    // Logging section
    if (!section || strcmp(section, "Logging") == 0) {
        format_section_header(header, sizeof(header), "D", "Logging");
        log_this("Config-Dump", "%s", LOG_LEVEL_STATE, header);
        dump_logging_config(&config->logging);
    }

    // WebServer section
    if (!section || strcmp(section, "WebServer") == 0) {
        format_section_header(header, sizeof(header), "E", "WebServer");
        log_this("Config-Dump", "%s", LOG_LEVEL_STATE, header);
        dump_webserver_config(&config->webserver);
    }

    // API section
    if (!section || strcmp(section, "API") == 0) {
        format_section_header(header, sizeof(header), "F", "API");
        log_this("Config-Dump", "%s", LOG_LEVEL_STATE, header);
        dump_api_config(&config->api);
    }

    // Swagger section
    if (!section || strcmp(section, "Swagger") == 0) {
        format_section_header(header, sizeof(header), "G", "Swagger");
        log_this("Config-Dump", "%s", LOG_LEVEL_STATE, header);
        if (section) log_this("Config", "――― Section dump not yet implemented", LOG_LEVEL_STATE);
    }

    // WebSocket section
    if (!section || strcmp(section, "WebSocket") == 0) {
        format_section_header(header, sizeof(header), "H", "WebSocket");
        log_this("Config-Dump", "%s", LOG_LEVEL_STATE, header);
        if (section) log_this("Config", "――― Section dump not yet implemented", LOG_LEVEL_STATE);
    }

    // Terminal section
    if (!section || strcmp(section, "Terminal") == 0) {
        format_section_header(header, sizeof(header), "I", "Terminal");
        log_this("Config-Dump", "%s", LOG_LEVEL_STATE, header);
        if (section) log_this("Config", "――― Section dump not yet implemented", LOG_LEVEL_STATE);
    }

    // mDNS Server section
    if (!section || strcmp(section, "mDNS Server") == 0) {
     
        format_section_header(header, sizeof(header), "J", "mDNS Server");
        log_this("Config-Dump", "%s", LOG_LEVEL_STATE, header);
        if (section) log_this("Config", "――― Section dump not yet implemented", LOG_LEVEL_STATE);
    }

    // mDNS Client section
    if (!section || strcmp(section, "mDNS Client") == 0) {
        format_section_header(header, sizeof(header), "K", "mDNS Client");
        log_this("Config-Dump", "%s", LOG_LEVEL_STATE, header);
        if (section) log_this("Config", "――― Section dump not yet implemented", LOG_LEVEL_STATE);
    }

    // Mail Relay section
    if (!section || strcmp(section, "Mail Relay") == 0) {
        format_section_header(header, sizeof(header), "L", "Mail Relay");
        log_this("Config-Dump", "%s", LOG_LEVEL_STATE, header);
        if (section) log_this("Config", "――― Section dump not yet implemented", LOG_LEVEL_STATE);
    }

    // Print section
    if (!section || strcmp(section, "Print") == 0) {
        format_section_header(header, sizeof(header), "M", "Print");
        log_this("Config-Dump", "%s", LOG_LEVEL_STATE, header);
        if (section) log_this("Config", "――― Section dump not yet implemented", LOG_LEVEL_STATE);
    }

    // Resources section
    if (!section || strcmp(section, "Resources") == 0) {
        format_section_header(header, sizeof(header), "N", "Resources");
        log_this("Config-Dump", "%s", LOG_LEVEL_STATE, header);
        if (section) log_this("Config", "――― Section dump not yet implemented", LOG_LEVEL_STATE);
    }

    // OIDC section
    if (!section || strcmp(section, "OIDC") == 0) {
        format_section_header(header, sizeof(header), "O", "OIDC");
        log_this("Config-Dump", "%s", LOG_LEVEL_STATE, header);
        if (section) log_this("Config", "――― Section dump not yet implemented", LOG_LEVEL_STATE);
    }

    // Notify section
    if (!section || strcmp(section, "Notify") == 0) {
        format_section_header(header, sizeof(header), "P", "Notify");
        log_this("Config-Dump", "%s", LOG_LEVEL_STATE, header);
        if (section) log_this("Config", "――― Section dump not yet implemented", LOG_LEVEL_STATE);
    }

    format_section_header(header, sizeof(header), "AppConfig Dump Complete", "");
    log_this("Config-Dump", "%s", LOG_LEVEL_STATE, header);

}

/*
 * Get the current application configuration
 * 
 * Returns a pointer to the current application configuration.
 * This configuration is loaded by load_config() and stored in a static variable.
 * The returned pointer should not be modified by the caller.
 * 
 * @return Pointer to the current application configuration
 */
const AppConfig* get_app_config(void) {
    return app_config;
}

/*
 * Clean up all resources allocated in the AppConfig structure
 *
 * This ensures all dynamically allocated memory within the AppConfig
 * structure is properly freed, preventing memory leaks.
 *
 * @param config The config structure to clean up
 */
static void clean_app_config(AppConfig* config) {
    if (!config) return;

    // Clean up configurations in A-P order
        
    cleanup_server_config(&config->server);            // A. Server Configuration
    cleanup_network_config(&config->network);          // B. Network Configuration
    cleanup_database_config(&config->databases);       // C. Database Configuration 
    cleanup_logging_config(&config->logging);          // D. Logging Configuration
    cleanup_webserver_config(&config->webserver);      // E. WebServer Configuration
    cleanup_api_config(&config->api);                  // F. API Configuration
    config_swagger_cleanup(config->swagger);           // G. Swagger Configuration (pointer)
    config_websocket_cleanup(&config->websocket);      // H. WebSocket Configuration
    config_terminal_cleanup(&config->terminal);        // I. Terminal Configuration
    config_mdns_server_cleanup(&config->mdns_server);  // J. mDNS Server Configuration
    config_mdns_client_cleanup(&config->mdns_client);  // K. mDNS Client Configuration
    config_mailrelay_cleanup(&config->mail_relay);     // L. Mail Relay Configuration
    config_print_cleanup(&config->print);              // M. Print Configuration
    config_resources_cleanup(&config->resources);      // N. Resources Configuration
    config_oidc_cleanup(&config->oidc);                // O. OIDC Configuration
    config_notify_cleanup(&config->notify);            // P. Notify Configuration

}

/*
 * Perform final cleanup of app configuration during shutdown
 * This function should be called during the shutdown sequence
 * to prevent memory leaks.
 */
void cleanup_application_config(void) {
    if (app_config) {
        log_this("Config", "Cleaning up application configuration", LOG_LEVEL_STATE);
        clean_app_config(app_config);
        free(app_config);
        app_config = NULL;
    }
}