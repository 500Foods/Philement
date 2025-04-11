/*
 * Terminal Configuration
 *
 * Defines the configuration structure and functions for the terminal subsystem.
 */

#ifndef CONFIG_TERMINAL_H
#define CONFIG_TERMINAL_H

#include <stdbool.h>
#include <jansson.h>
#include "config_forward.h"  // For AppConfig forward declaration

/**
 * Terminal configuration structure
 */
typedef struct TerminalConfig {
    bool enabled;                  /**< Whether terminal access is enabled */
    char* web_path;               /**< Web path for terminal access */
    char* shell_command;          /**< Shell command to execute */
    int max_sessions;             /**< Maximum concurrent terminal sessions */
    int idle_timeout_seconds;     /**< Session idle timeout in seconds */
} TerminalConfig;

/**
 * Load terminal configuration from JSON
 * 
 * @param root The JSON root object containing configuration
 * @param config The application configuration structure to populate
 * @return true on success, false on failure
 */
bool load_terminal_config(json_t* root, AppConfig* config);

/**
 * Initialize terminal configuration with defaults
 * 
 * @param config The configuration structure to initialize
 * @return 0 on success, -1 on failure
 */
int config_terminal_init(TerminalConfig* config);

/**
 * Free resources allocated by terminal configuration
 * 
 * This function frees all resources allocated by config_terminal_init.
 * It safely handles NULL pointers and partial initialization.
 * 
 * @param config The configuration structure to cleanup
 */
void config_terminal_cleanup(TerminalConfig* config);

/**
 * Validate terminal configuration values
 * 
 * @param config The configuration structure to validate
 * @return 0 if valid, -1 if invalid
 */
int config_terminal_validate(const TerminalConfig* config);

#endif /* CONFIG_TERMINAL_H */