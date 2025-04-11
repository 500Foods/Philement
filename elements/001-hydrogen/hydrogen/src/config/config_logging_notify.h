/*
 * Notify Logging Configuration
 *
 * Defines the configuration structure for notification-based logging output.
 * This coordinates with the Notify subsystem to send important log messages
 * through notification channels (e.g., SMTP).
 */

#ifndef HYDROGEN_CONFIG_LOGGING_NOTIFY_H
#define HYDROGEN_CONFIG_LOGGING_NOTIFY_H

#include <stdbool.h>
#include <stddef.h>
#include "config_forward.h"
#include "config_logging_console.h"  // For SubsystemConfig definition

// Log level definitions - match logging.h values
#define LOG_LEVEL_TRACE      0  /* Log everything - special value */
#define LOG_LEVEL_DEBUG    1  /* Debug-level messages */
#define LOG_LEVEL_STATE     2  /* General information, normal operation */
#define LOG_LEVEL_ALERT  3  /* Warning conditions */
#define LOG_LEVEL_ERROR    4  /* Error conditions */
#define LOG_LEVEL_FATAL 5  /* Critical conditions */
#define LOG_LEVEL_QUIET     6  /* Log nothing - special value */

// Notify logging configuration structure
typedef struct LoggingNotifyConfig {
    bool enabled;                // Whether notify logging is enabled
    int default_level;          // Default log level for notify output
    SubsystemConfig* subsystems;  // Array of subsystem configurations
    size_t subsystem_count;     // Number of configured subsystems
} LoggingNotifyConfig;

/*
 * Initialize notify logging configuration with default values
 *
 * This function initializes a new LoggingNotifyConfig structure with
 * default values that provide a secure baseline configuration.
 *
 * @param config Pointer to LoggingNotifyConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 * - If memory allocation fails
 */
int config_logging_notify_init(LoggingNotifyConfig* config);

/*
 * Free resources allocated for notify logging configuration
 *
 * This function frees all resources allocated by config_logging_notify_init.
 * It safely handles NULL pointers and partial initialization.
 *
 * @param config Pointer to LoggingNotifyConfig structure to cleanup
 */
void config_logging_notify_cleanup(LoggingNotifyConfig* config);

/*
 * Validate notify logging configuration values
 *
 * This function performs validation of the configuration:
 * - Verifies default_level is valid
 * - Validates all subsystem levels
 * - Checks subsystem name validity
 *
 * @param config Pointer to LoggingNotifyConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If default_level is invalid
 * - If any subsystem configuration is invalid
 */
int config_logging_notify_validate(const LoggingNotifyConfig* config);

/*
 * Get the configured log level for a subsystem
 *
 * This function returns the configured log level for the specified subsystem,
 * falling back to the default level if no specific configuration exists.
 *
 * @param config Pointer to LoggingNotifyConfig structure
 * @param subsystem Name of the subsystem to check
 * @return Configured log level for the subsystem
 */
int get_subsystem_level_notify(const LoggingNotifyConfig* config, const char* subsystem);

#endif /* HYDROGEN_CONFIG_LOGGING_NOTIFY_H */