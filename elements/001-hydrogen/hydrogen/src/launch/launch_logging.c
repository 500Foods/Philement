/*
 * Logging Subsystem Launch Implementation
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "launch.h"

volatile sig_atomic_t logging_stopping = 0;

// Check logging subsystem launch readiness
LaunchReadiness check_logging_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = false;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup(SR_LOGGING));

    // Early return cases
    if (server_stopping) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   System shutdown in progress"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_LOGGING, .ready = false, .messages = messages };
    }

    if (!server_starting && !server_running) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   System not in startup or running state"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_LOGGING, .ready = false, .messages = messages };
    }

    if (!app_config) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Configuration not loaded"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_LOGGING, .ready = false, .messages = messages };
    }

    // Get logging configuration
    const LoggingConfig* log_config = &app_config->logging;
    bool config_valid = true;

    // Validate log levels
    if (!log_config->levels || log_config->level_count == 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   No log levels defined"));
        config_valid = false;
    } else {
        char* level_count_msg = malloc(256);
        if (level_count_msg) {
            snprintf(level_count_msg, 256, "  Go:      Log levels defined: %zu", log_config->level_count);
            add_launch_message(&messages, &count, &capacity, level_count_msg);
        }

        // Check each level
        for (size_t i = 0; i < log_config->level_count; i++) {
            if (log_config->levels[i].value < LOG_LEVEL_TRACE ||
                log_config->levels[i].value > LOG_LEVEL_QUIET ||
                !log_config->levels[i].name) {
                char* invalid_level_msg = malloc(256);
                if (invalid_level_msg) {
                    snprintf(invalid_level_msg, 256, "  No-Go:   Invalid log level at index %zu", i);
                    add_launch_message(&messages, &count, &capacity, invalid_level_msg);
                }
                config_valid = false;
            }
        }
    }

    // Validate console logging
    if (log_config->console.enabled) {
        if (log_config->console.default_level < LOG_LEVEL_TRACE ||
            log_config->console.default_level > LOG_LEVEL_QUIET) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid console default level"));
            config_valid = false;
        } else {
            char* console_msg = malloc(256);
            if (console_msg) {
                snprintf(console_msg, 256, "  Go:      Console logging enabled at level %s",
                        config_logging_get_level_name(log_config, log_config->console.default_level));
                add_launch_message(&messages, &count, &capacity, console_msg);
            }
        }
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Console logging disabled"));
    }

    // Validate file logging
    if (log_config->file.enabled) {
        if (log_config->file.default_level < LOG_LEVEL_TRACE ||
            log_config->file.default_level > LOG_LEVEL_QUIET) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid file default level"));
            config_valid = false;
        } else {
            char* file_msg = malloc(256);
            if (file_msg) {
                snprintf(file_msg, 256, "  Go:      File logging enabled at level %s",
                        config_logging_get_level_name(log_config, log_config->file.default_level));
                add_launch_message(&messages, &count, &capacity, file_msg);
            }
        }
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      File logging disabled"));
    }

    // Validate database logging
    if (log_config->database.enabled) {
        if (log_config->database.default_level < LOG_LEVEL_TRACE ||
            log_config->database.default_level > LOG_LEVEL_QUIET) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid database default level"));
            config_valid = false;
        } else {
            char* db_msg = malloc(256);
            if (db_msg) {
                snprintf(db_msg, 256, "  Go:      Database logging enabled at level %s",
                        config_logging_get_level_name(log_config, log_config->database.default_level));
                add_launch_message(&messages, &count, &capacity, db_msg);
            }
        }
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Database logging disabled"));
    }

    // Validate notification logging
    if (log_config->notify.enabled) {
        if (log_config->notify.default_level < LOG_LEVEL_TRACE ||
            log_config->notify.default_level > LOG_LEVEL_QUIET) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid notification default level"));
            config_valid = false;
        } else {
            char* notify_msg = malloc(256);
            if (notify_msg) {
                snprintf(notify_msg, 256, "  Go:      Notification logging enabled at level %s",
                        config_logging_get_level_name(log_config, log_config->notify.default_level));
                add_launch_message(&messages, &count, &capacity, notify_msg);
            }
        }
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Notification logging disabled"));
    }

    // Ensure at least one logging destination is enabled
    if (!log_config->console.enabled && !log_config->file.enabled &&
        !log_config->database.enabled && !log_config->notify.enabled) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   No logging destinations enabled"));
        config_valid = false;
    }

    // Basic readiness check - verify subsystem registration and config validity
    int subsystem_id = get_subsystem_id_by_name(SR_LOGGING);
    if (subsystem_id >= 0 && config_valid) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      " SR_LOGGING " subsystem registered"));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  Go For Launch of " SR_LOGGING " subsystem"));
        overall_readiness = true;
    } else {
        if (subsystem_id < 0) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   " SR_LOGGING " subsystem not registered"));
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of " SR_LOGGING " subsystem"));
        overall_readiness = false;
    }

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = SR_LOGGING,
        .ready = overall_readiness,
        .messages = messages
    };
}

// Launch the logging subsystem
int launch_logging_subsystem(void) {
    
    logging_stopping = 0;
    
    log_this(SR_LOGGING, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_LOGGING, "LAUNCH: " SR_LOGGING, LOG_LEVEL_STATE, 0);

    // Get subsystem ID and update state
    int subsystem_id = get_subsystem_id_by_name(SR_LOGGING);
    if (subsystem_id >= 0) {
        update_subsystem_state(subsystem_id, SUBSYSTEM_RUNNING);
        log_this(SR_LOGGING, SR_LOGGING " subsystem initialized", LOG_LEVEL_STATE, 0);
        return 1;
    }
    
    log_this(SR_LOGGING, "Failed to initialize " SR_LOGGING " subsystem", LOG_LEVEL_ERROR, 0);
    return 0;
}

// Shutdown handler - defined in launch_logging.h, implemented here
void shutdown_logging(void) {
    if (!logging_stopping) {
        logging_stopping = 1;
        log_this(SR_LOGGING, SR_LOGGING " subsystem shutting down", LOG_LEVEL_STATE, 0);
    }
}
