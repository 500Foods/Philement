/*
 * Terminal configuration JSON parsing
 */

#include <jansson.h>
#include <stdbool.h>
#include <string.h>
#include "../../logging/logging.h"
#include "../config.h"
#include "../config_utils.h"
#include "json_terminal.h"  // This references the header in the same directory
#include "../logging/config_logging_utils.h"

bool load_json_terminal(json_t* root, AppConfig* config __attribute__((unused))) {
    // Terminal Configuration
    json_t* terminal = json_object_get(root, "Terminal");
    if (json_is_object(terminal)) {
        log_config_section_header("Terminal");
        
        json_t* enabled = json_object_get(terminal, "Enabled");
        bool terminal_enabled = get_config_bool(enabled, true);
        log_config_section_item("Enabled", "%s", LOG_LEVEL_STATE, !enabled, 0, NULL, NULL, "Config", 
                            terminal_enabled ? "true" : "false");
        
        json_t* web_path = json_object_get(terminal, "WebPath");
        char* terminal_path = get_config_string_with_env("WebPath", web_path, "/terminal");
        log_config_section_item("WebPath", "%s", LOG_LEVEL_STATE, !web_path, 0, NULL, NULL, "Config", terminal_path);
        free(terminal_path);
        
        json_t* shell_cmd = json_object_get(terminal, "ShellCommand");
        char* terminal_shell = get_config_string_with_env("ShellCommand", shell_cmd, "/bin/bash");
        log_config_section_item("ShellCommand", "%s", LOG_LEVEL_STATE, !shell_cmd, 0, NULL, NULL, "Config", terminal_shell);
        free(terminal_shell);
        
        json_t* max_sessions = json_object_get(terminal, "MaxSessions");
        if (json_is_integer(max_sessions)) {
            log_config_section_item("MaxSessions", "%d", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config", 
                json_integer_value(max_sessions));
        } else {
            log_config_section_item("MaxSessions", "%d", LOG_LEVEL_STATE, 1, 0, NULL, NULL, "Config", 4);
        }
        
        json_t* idle_timeout = json_object_get(terminal, "IdleTimeoutSeconds");
        if (json_is_integer(idle_timeout)) {
            log_config_section_item("IdleTimeoutSeconds", "%d", LOG_LEVEL_STATE, 0, 0, NULL, NULL, "Config", 
                json_integer_value(idle_timeout));
        }
    } else {
        log_config_section_header("Terminal");
        log_config_section_item("Status", "Section missing", LOG_LEVEL_ALERT, 1, 0, NULL, NULL, "Config");
    }
    
    // Always return true as there's no failure condition here
    return true;
}