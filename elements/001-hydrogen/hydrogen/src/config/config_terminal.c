/*
 * Terminal Configuration Implementation
 *
 * Implements the configuration handlers for the terminal subsystem,
 * including JSON parsing, environment variable handling, and validation.
 */

#include <jansson.h>
#include <stdbool.h>
#include <string.h>
#include "config.h"
#include "config_utils.h"
#include "config_terminal.h"
#include "../logging/logging.h"

bool load_terminal_config(json_t* root, AppConfig* config) {
    bool success = true;
    
    // Terminal Configuration
    PROCESS_SECTION(root, "Terminal"); {
        // Process configuration values
        PROCESS_BOOL(root, &config->terminal, enabled, "Terminal/Enabled", "Terminal");
        PROCESS_STRING(root, &config->terminal, web_path, "Terminal/WebPath", "Terminal");
        PROCESS_STRING(root, &config->terminal, shell_command, "Terminal/ShellCommand", "Terminal");
        PROCESS_INT(root, &config->terminal, max_sessions, "Terminal/MaxSessions", "Terminal");
        PROCESS_INT(root, &config->terminal, idle_timeout_seconds, "Terminal/IdleTimeoutSeconds", "Terminal");
    }
    
    return success;
}

int config_terminal_init(TerminalConfig* config) {
    if (!config) {
        return -1;
    }
    
    // Initialize with defaults
    config->enabled = true;
    config->web_path = strdup("/terminal");
    config->shell_command = strdup("/bin/bash");
    config->max_sessions = 4;
    config->idle_timeout_seconds = 300;
    
    return 0;
}

void config_terminal_cleanup(TerminalConfig* config) {
    if (!config) {
        return;
    }
    
    // Free allocated strings
    free(config->web_path);
    free(config->shell_command);
    
    // Reset to defaults
    config->enabled = false;
    config->web_path = NULL;
    config->shell_command = NULL;
    config->max_sessions = 0;
    config->idle_timeout_seconds = 0;
}

int config_terminal_validate(const TerminalConfig* config) {
    if (!config) {
        return -1;
    }
    
    // Validate required strings
    if (!config->web_path || !config->shell_command) {
        return -1;
    }
    
    // Validate numeric ranges
    if (config->max_sessions < 1 || config->max_sessions > 100) {
        return -1;
    }
    
    if (config->idle_timeout_seconds < 60 || config->idle_timeout_seconds > 3600) {
        return -1;
    }
    
    return 0;
}