/*
 * Terminal Configuration Implementation
 *
 * Implements the configuration handlers for the terminal subsystem,
 * including JSON parsing and environment variable handling.
 */

#include <stdlib.h>
#include <string.h>
#include "config_terminal.h"
#include "config.h"
#include "config_utils.h"
#include "../logging/logging.h"

bool load_terminal_config(json_t* root, AppConfig* config) {
    // Initialize with defaults
    TerminalConfig* terminal = &config->terminal;
    terminal->enabled = true;  // Enable by default for better UX
    terminal->max_sessions = 4;  // Conservative default
    terminal->idle_timeout_seconds = 300;  // 5 minutes idle timeout
    
    // Initialize string fields with defaults
    terminal->web_path = strdup("/terminal");  // Standard web path
    if (!terminal->web_path) {
        log_this("Terminal", "Failed to allocate web path string", LOG_LEVEL_ERROR);
        return false;
    }

    terminal->shell_command = strdup("/bin/bash");  // Default shell
    if (!terminal->shell_command) {
        log_this("Terminal", "Failed to allocate shell command string", LOG_LEVEL_ERROR);
        free(terminal->web_path);
        terminal->web_path = NULL;
        return false;
    }

    // Process configuration values
    bool success = true;

    // Process main section and enabled flag
    success = PROCESS_SECTION(root, "Terminal");
    success = success && PROCESS_BOOL(root, terminal, enabled, "Terminal.Enabled", "Terminal");
    
    
        success = success && PROCESS_STRING(root, terminal, web_path, "Terminal.WebPath", "Terminal");
        success = success && PROCESS_STRING(root, terminal, shell_command, "Terminal.ShellCommand", "Terminal");
        success = success && PROCESS_INT(root, terminal, max_sessions, "Terminal.MaxSessions", "Terminal");
        success = success && PROCESS_INT(root, terminal, idle_timeout_seconds, "Terminal.IdleTimeoutSeconds", "Terminal");
    
    return success;
}

void cleanup_terminal_config(TerminalConfig* config) {
    if (!config) {
        return;
    }

    free(config->web_path);
    free(config->shell_command);
    memset(config, 0, sizeof(TerminalConfig));
}

void dump_terminal_config(const TerminalConfig* config) {
    if (!config) {
        DUMP_TEXT("", "Cannot dump NULL terminal config");
        return;
    }

    // Dump enabled status
    DUMP_BOOL2("――", "Enabled", config->enabled);
    
        char value_str[256];
        
        // Web Path
        snprintf(value_str, sizeof(value_str), "Web Path: %s", 
                config->web_path ? config->web_path : "(not set)");
        DUMP_TEXT("――", value_str);
        
        // Shell Command
        snprintf(value_str, sizeof(value_str), "Shell Command: %s",
                config->shell_command ? config->shell_command : "(not set)");
        DUMP_TEXT("――", value_str);
        
        // Max Sessions
        snprintf(value_str, sizeof(value_str), "Max Sessions: %d", 
                config->max_sessions);
        DUMP_TEXT("――", value_str);
        
        // Idle Timeout
        snprintf(value_str, sizeof(value_str), "Idle Timeout: %d seconds",
                config->idle_timeout_seconds);
        DUMP_TEXT("――", value_str);
    
}