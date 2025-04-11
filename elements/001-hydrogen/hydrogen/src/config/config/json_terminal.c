/*
 * Terminal configuration JSON parsing
 */

#include <jansson.h>
#include <stdbool.h>
#include <string.h>
#include "../../logging/logging.h"
#include "../config.h"
#include "../config_utils.h"
#include "json_terminal.h"

bool load_json_terminal(json_t* root, AppConfig* config __attribute__((unused))) {
    // Terminal Configuration
    json_t* terminal = json_object_get(root, "Terminal");
    bool using_defaults = !json_is_object(terminal);
    
    log_config_section("Terminal", using_defaults);
        
    if (!using_defaults) {
        // Process enabled flag
        json_t* enabled = json_object_get(terminal, "Enabled");
        bool terminal_enabled = get_config_bool(enabled, true);
        log_config_item("Enabled", terminal_enabled ? "true" : "false", !enabled, 0);
        
        // Process web path
        json_t* web_path = json_object_get(terminal, "WebPath");
        char* terminal_path = get_config_string_with_env("WebPath", web_path, "/terminal");
        log_config_item("WebPath", terminal_path, !web_path, 0);
        free(terminal_path);
        
        // Process shell command
        json_t* shell_cmd = json_object_get(terminal, "ShellCommand");
        char* terminal_shell = get_config_string_with_env("ShellCommand", shell_cmd, "/bin/bash");
        log_config_item("ShellCommand", terminal_shell, !shell_cmd, 0);
        free(terminal_shell);
        
        // Process max sessions
        json_t* max_sessions = json_object_get(terminal, "MaxSessions");
        bool using_default_max = !json_is_integer(max_sessions);
        int max_sessions_val = using_default_max ? 4 : json_integer_value(max_sessions);
        log_config_item("MaxSessions", format_int_buffer(max_sessions_val), using_default_max, 0);
        
        // Process idle timeout
        json_t* idle_timeout = json_object_get(terminal, "IdleTimeoutSeconds");
        if (json_is_integer(idle_timeout)) {
            log_config_item("IdleTimeoutSeconds", format_int_buffer(json_integer_value(idle_timeout)), false, 0);
        }
    } else {
        log_config_item("Status", "Section missing, using defaults", true, 0);
    }
    
    // Always return true as there's no failure condition here
    return true;
}