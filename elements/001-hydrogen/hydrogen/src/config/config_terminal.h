/*
 * Terminal Configuration
 *
 * Defines the configuration structure and functions for the terminal subsystem.
 * Handles configuration of terminal access, web paths, shell commands, and
 * session management settings.
 */

#ifndef CONFIG_TERMINAL_H
#define CONFIG_TERMINAL_H

#include <stdbool.h>
#include <jansson.h>
#include "config_forward.h"

/**
 * Terminal configuration structure
 */
typedef struct TerminalConfig {
    bool enabled;                  /**< Whether terminal access is enabled */
    char* web_path;               /**< Web path for terminal access */
    char* shell_command;          /**< Shell command to execute */
    int max_sessions;             /**< Maximum concurrent terminal sessions */
    int idle_timeout_seconds;     /**< Session idle timeout in seconds */
    int buffer_size;              /**< PTY read buffer size in bytes */

    // NEW: WebRoot support for PAYLOAD:/ paths or filesystem paths
    char* webroot;                 /** NEW: PAYLOAD:/terminal or /filesystem/path */
    char* cors_origin;             /** NEW: Optional per-subsystem CORS override */
    char* index_page;              /** NEW: Configurable index page (default: "terminal.html") */
} TerminalConfig;

/**
 * Load terminal configuration from JSON
 * 
 * Loads and processes terminal configuration settings from the provided JSON,
 * applying environment variable overrides and defaults as needed.
 * 
 * @param root The JSON root object containing configuration
 * @param config The application configuration structure to populate
 * @return true on success, false on failure
 */
bool load_terminal_config(json_t* root, AppConfig* config);

/**
 * Clean up terminal configuration
 * 
 * Frees all resources allocated for terminal configuration.
 * Safely handles NULL pointers and partial initialization.
 * 
 * @param config The configuration structure to cleanup
 */
void cleanup_terminal_config(TerminalConfig* config);

/**
 * Dump terminal configuration for debugging
 * 
 * Outputs the current terminal configuration state in a structured format,
 * matching the JSON structure and showing environment variable usage.
 * 
 * @param config The configuration structure to dump
 */
void dump_terminal_config(const TerminalConfig* config);

#endif /* CONFIG_TERMINAL_H */
