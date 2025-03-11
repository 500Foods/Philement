/*
 * Console Logging Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include "config_logging_console.h"

int config_logging_console_init(LoggingConsoleConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize basic settings
    config->enabled = DEFAULT_CONSOLE_ENABLED;
    config->default_level = DEFAULT_CONSOLE_LOG_LEVEL;

    // Initialize subsystem log levels
    config->subsystems.thread_mgmt = DEFAULT_CONSOLE_THREAD_MGMT_LEVEL;
    config->subsystems.shutdown = DEFAULT_CONSOLE_SHUTDOWN_LEVEL;
    config->subsystems.mdns_server = DEFAULT_CONSOLE_MDNS_SERVER_LEVEL;
    config->subsystems.web_server = DEFAULT_CONSOLE_WEB_SERVER_LEVEL;
    config->subsystems.websocket = DEFAULT_CONSOLE_WEBSOCKET_LEVEL;
    config->subsystems.print_queue = DEFAULT_CONSOLE_PRINT_QUEUE_LEVEL;
    config->subsystems.log_queue_mgr = DEFAULT_CONSOLE_LOG_QUEUE_LEVEL;

    return 0;
}

void config_logging_console_cleanup(LoggingConsoleConfig* config) {
    if (!config) {
        return;
    }

    // Zero out the structure
    memset(config, 0, sizeof(LoggingConsoleConfig));
}

static int validate_log_level(int level) {
    return level >= MIN_LOG_LEVEL && level <= MAX_LOG_LEVEL;
}

static int validate_subsystem_levels(const LoggingConsoleConfig* config) {
    // Validate all subsystem log levels
    return validate_log_level(config->subsystems.thread_mgmt) &&
           validate_log_level(config->subsystems.shutdown) &&
           validate_log_level(config->subsystems.mdns_server) &&
           validate_log_level(config->subsystems.web_server) &&
           validate_log_level(config->subsystems.websocket) &&
           validate_log_level(config->subsystems.print_queue) &&
           validate_log_level(config->subsystems.log_queue_mgr);
}

int config_logging_console_validate(const LoggingConsoleConfig* config) {
    if (!config) {
        return -1;
    }

    // If console logging is enabled, validate all settings
    if (config->enabled) {
        // Validate default log level
        if (!validate_log_level(config->default_level)) {
            return -1;
        }

        // Validate subsystem log levels
        if (!validate_subsystem_levels(config)) {
            return -1;
        }

        // Validate relationships between log levels
        
        // Critical subsystems should not have lower log level than default
        if (config->subsystems.shutdown < config->default_level ||
            config->subsystems.thread_mgmt < config->default_level) {
            return -1;
        }

        // Log queue manager should have at least info level when enabled
        if (config->subsystems.log_queue_mgr < 2) {  // 2 = Info level
            return -1;
        }

        // Web server and WebSocket should have matching log levels
        // since they are tightly coupled
        if (config->subsystems.web_server != config->subsystems.websocket) {
            return -1;
        }
    }

    return 0;
}