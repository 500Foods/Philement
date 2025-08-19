/*
 * Logging Subsystem Launch Implementation
 */

// Global includes 
#include "../hydrogen.h"

// Local includes
#include "launch.h"

volatile sig_atomic_t logging_stopping = 0;

// Helper functions for message formatting
static void add_message(const char** messages, int* count, const char* message) {
    if (message) {
        messages[*count] = message;
        (*count)++;
    }
}

static void add_go_message(const char** messages, int* count, const char* prefix, const char* format, ...) {
    char temp[256];
    va_list args;
    va_start(args, format);
    vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);
    
    char buffer[512];
    if (strcmp(prefix, "No-Go") == 0) {
        snprintf(buffer, sizeof(buffer), "  %s:   %s", prefix, temp);
    } else {
        snprintf(buffer, sizeof(buffer), "  %s:      %s", prefix, temp);
    }
    
    add_message(messages, count, strdup(buffer));
}

static void add_decision_message(const char** messages, int* count, const char* format, ...) {
    char temp[256];
    va_list args;
    va_start(args, format);
    vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);
    
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "  Decide:  %s", temp);
    add_message(messages, count, strdup(buffer));
}

// Check logging subsystem launch readiness
LaunchReadiness check_logging_launch_readiness(void) {
    bool overall_readiness = false;
    
    // Allocate space for messages
    const char** messages = malloc(10 * sizeof(const char*));
    if (!messages) {
        return (LaunchReadiness){0};
    }
    int msg_count = 0;
    
    // Add the subsystem name as the first message
    add_message(messages, &msg_count, strdup("Logging"));
    
    // Early return cases with cleanup
    if (server_stopping) {
        add_go_message(messages, &msg_count, "No-Go", "System shutdown in progress");
        messages[msg_count] = NULL;
        return (LaunchReadiness){
            .subsystem = "Logging",
            .ready = false,
            .messages = messages
        };
    }
    
    if (!server_starting && !server_running) {
        add_go_message(messages, &msg_count, "No-Go", "System not in startup or running state");
        messages[msg_count] = NULL;
        return (LaunchReadiness){
            .subsystem = "Logging",
            .ready = false,
            .messages = messages
        };
    }
    
    const AppConfig* config = get_app_config();
    if (!config) {
        add_go_message(messages, &msg_count, "No-Go", "Configuration not loaded");
        messages[msg_count] = NULL;
        return (LaunchReadiness){
            .subsystem = "Logging",
            .ready = false,
            .messages = messages
        };
    }
    
    // Get logging configuration
    const LoggingConfig* log_config = &config->logging;
    bool config_valid = true;

    // Validate log levels
    if (!log_config->levels || log_config->level_count == 0) {
        add_go_message(messages, &msg_count, "No-Go", "No log levels defined");
        config_valid = false;
    } else {
        add_go_message(messages, &msg_count, "Go", "Log levels defined: %zu", log_config->level_count);
        
        // Check each level
        for (size_t i = 0; i < log_config->level_count; i++) {
            if (log_config->levels[i].value < LOG_LEVEL_TRACE || 
                log_config->levels[i].value > LOG_LEVEL_QUIET ||
                !log_config->levels[i].name) {
                add_go_message(messages, &msg_count, "No-Go", "Invalid log level at index %zu", i);
                config_valid = false;
            }
        }
    }

    // Validate console logging
    if (log_config->console.enabled) {
        if (log_config->console.default_level < LOG_LEVEL_TRACE || 
            log_config->console.default_level > LOG_LEVEL_QUIET) {
            add_go_message(messages, &msg_count, "No-Go", "Invalid console default level");
            config_valid = false;
        } else {
            add_go_message(messages, &msg_count, "Go", "Console logging enabled at level %s", 
                          config_logging_get_level_name(log_config, log_config->console.default_level));
        }
    } else {
        add_go_message(messages, &msg_count, "Go", "Console logging disabled");
    }

    // Validate file logging
    if (log_config->file.enabled) {
        if (log_config->file.default_level < LOG_LEVEL_TRACE || 
            log_config->file.default_level > LOG_LEVEL_QUIET) {
            add_go_message(messages, &msg_count, "No-Go", "Invalid file default level");
            config_valid = false;
        } else {
            add_go_message(messages, &msg_count, "Go", "File logging enabled at level %s",
                          config_logging_get_level_name(log_config, log_config->file.default_level));
        }
    } else {
        add_go_message(messages, &msg_count, "Go", "File logging disabled");
    }

    // Validate database logging
    if (log_config->database.enabled) {
        if (log_config->database.default_level < LOG_LEVEL_TRACE || 
            log_config->database.default_level > LOG_LEVEL_QUIET) {
            add_go_message(messages, &msg_count, "No-Go", "Invalid database default level");
            config_valid = false;
        } else {
            add_go_message(messages, &msg_count, "Go", "Database logging enabled at level %s",
                          config_logging_get_level_name(log_config, log_config->database.default_level));
        }
    } else {
        add_go_message(messages, &msg_count, "Go", "Database logging disabled");
    }

    // Validate notification logging
    if (log_config->notify.enabled) {
        if (log_config->notify.default_level < LOG_LEVEL_TRACE || 
            log_config->notify.default_level > LOG_LEVEL_QUIET) {
            add_go_message(messages, &msg_count, "No-Go", "Invalid notification default level");
            config_valid = false;
        } else {
            add_go_message(messages, &msg_count, "Go", "Notification logging enabled at level %s",
                          config_logging_get_level_name(log_config, log_config->notify.default_level));
        }
    } else {
        add_go_message(messages, &msg_count, "Go", "Notification logging disabled");
    }

    // Ensure at least one logging destination is enabled
    if (!log_config->console.enabled && !log_config->file.enabled && 
        !log_config->database.enabled && !log_config->notify.enabled) {
        add_go_message(messages, &msg_count, "No-Go", "No logging destinations enabled");
        config_valid = false;
    }

    // Basic readiness check - verify subsystem registration and config validity
    int subsystem_id = get_subsystem_id_by_name("Logging");
    if (subsystem_id >= 0 && config_valid) {
        add_go_message(messages, &msg_count, "Go", "Logging subsystem registered");
        add_decision_message(messages, &msg_count, "Go For Launch of Logging Subsystem");
        overall_readiness = true;
    } else {
        if (subsystem_id < 0) {
            add_go_message(messages, &msg_count, "No-Go", "Logging subsystem not registered");
        }
        add_decision_message(messages, &msg_count, "No-Go For Launch of Logging Subsystem");
        overall_readiness = false;
    }
    
    messages[msg_count] = NULL;
    return (LaunchReadiness){
        .subsystem = "Logging",
        .ready = overall_readiness,
        .messages = messages
    };
}

// Launch the logging subsystem
int launch_logging_subsystem(void) {
    // Reset shutdown flag
    logging_stopping = 0;
    
    log_this("Logging", "Initializing logging subsystem", LOG_LEVEL_STATE);
    
    // Get subsystem ID and update state
    int subsystem_id = get_subsystem_id_by_name("Logging");
    if (subsystem_id >= 0) {
        update_subsystem_state(subsystem_id, SUBSYSTEM_RUNNING);
        log_this("Logging", "Logging subsystem initialized", LOG_LEVEL_STATE);
        return 1;
    }
    
    log_this("Logging", "Failed to initialize logging subsystem", LOG_LEVEL_ERROR);
    return 0;
}

// Shutdown handler - defined in launch_logging.h, implemented here
void shutdown_logging(void) {
    if (!logging_stopping) {
        logging_stopping = 1;
        log_this("Logging", "Logging subsystem shutting down", LOG_LEVEL_STATE);
    }
}
