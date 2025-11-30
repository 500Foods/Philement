/*
 * Terminal Configuration Implementation
 *
 * Implements the configuration handlers for the terminal subsystem,
 * including JSON parsing and environment variable handling.
 */

 // Global includes 
#include <src/hydrogen.h>

// Local includes
#include "config_terminal.h"

bool load_terminal_config(json_t* root, AppConfig* config) {
    // Initialize with defaults
    TerminalConfig* terminal = &config->terminal;
    terminal->enabled = true;  // Enable by default for better UX
    terminal->max_sessions = 4;  // Conservative default
    terminal->idle_timeout_seconds = 300;  // 5 minutes idle timeout
    terminal->buffer_size = 512;  // 512 bytes - small enough to fit in WebSocket with JSON overhead
    
    // Initialize string fields with defaults
    terminal->web_path = strdup("/terminal");  // Standard web path
    if (!terminal->web_path) {
        log_this(SR_TERMINAL, "Failed to allocate web path string", LOG_LEVEL_ERROR, 0);
        return false;
    }

    terminal->shell_command = strdup("/bin/zsh");  // Default shell
    if (!terminal->shell_command) {
        // Fallback to bash if zsh allocation fails
        terminal->shell_command = strdup("/bin/bash");
        if (!terminal->shell_command) {
            log_this(SR_TERMINAL, "Failed to allocate shell command string", LOG_LEVEL_ERROR, 0);
            free(terminal->web_path);
            terminal->web_path = NULL;
            return false;
        }
    }

    // NEW: Initialize WebRoot and CORS fields
    terminal->webroot = strdup("PAYLOAD:/terminal");  // Default WebRoot
    if (!terminal->webroot) {
        log_this(SR_TERMINAL, "Failed to allocate webroot string", LOG_LEVEL_ERROR, 0);
        free(terminal->web_path);
        free(terminal->shell_command);
        terminal->web_path = NULL;
        terminal->shell_command = NULL;
        return false;
    }

    terminal->cors_origin = strdup("*");  // NEW: Allow all origins by default

    terminal->index_page = strdup("terminal.html");  // NEW: Default index page
    if (!terminal->index_page) {
        log_this(SR_TERMINAL, "Failed to allocate index page string", LOG_LEVEL_ERROR, 0);
        free(terminal->web_path);
        free(terminal->shell_command);
        free(terminal->webroot);
        free(terminal->cors_origin);
        terminal->web_path = NULL;
        terminal->shell_command = NULL;
        terminal->webroot = NULL;
        terminal->cors_origin = NULL;
        return false;
    }

    // Process configuration values
    bool success = true;

    // Process main section and enabled flag
    success = PROCESS_SECTION(root, SR_TERMINAL);
    success = success && PROCESS_BOOL(root, terminal, enabled, SR_TERMINAL ".Enabled", SR_TERMINAL);
    
    
        success = success && PROCESS_STRING(root, terminal, web_path, SR_TERMINAL ".WebPath", SR_TERMINAL);
        success = success && PROCESS_STRING(root, terminal, shell_command, SR_TERMINAL ".ShellCommand", SR_TERMINAL);
        success = success && PROCESS_INT(root, terminal, max_sessions, SR_TERMINAL ".MaxSessions", SR_TERMINAL);
        success = success && PROCESS_INT(root, terminal, idle_timeout_seconds, SR_TERMINAL ".IdleTimeoutSeconds", SR_TERMINAL);
        success = success && PROCESS_INT(root, terminal, buffer_size, SR_TERMINAL ".BufferSize", SR_TERMINAL);
// NEW: Process WebRoot, CORS, and IndexPage fields
success = success && PROCESS_STRING(root, terminal, webroot, SR_TERMINAL ".WebRoot", SR_TERMINAL);
success = success && PROCESS_STRING(root, terminal, cors_origin, SR_TERMINAL ".CORSOrigin", SR_TERMINAL);
success = success && PROCESS_STRING(root, terminal, index_page, SR_TERMINAL ".IndexPage", SR_TERMINAL);

    
    return success;
}

void cleanup_terminal_config(TerminalConfig* config) {
    if (!config) {
        return;
    }

    free(config->web_path);
    free(config->shell_command);
    free(config->webroot);           // NEW
    free(config->cors_origin);       // NEW
    free(config->index_page);        // NEW
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
        
        // Buffer Size
        snprintf(value_str, sizeof(value_str), "Buffer Size: %d bytes",
                config->buffer_size);
        DUMP_TEXT("――", value_str);

        // NEW: WebRoot
        snprintf(value_str, sizeof(value_str), "WebRoot: %s",
                config->webroot ? config->webroot : "(not set)");
        DUMP_TEXT("――", value_str);
// NEW: CORS Origin
DUMP_STRING2("――", "CORS Origin", config->cors_origin);

// NEW: Index Page
DUMP_STRING2("――", "Index Page", config->index_page);

    
}
