/*
 * Notify Configuration
 *
 * Defines the configuration structure and handlers for the notification subsystem.
 * Includes settings for notification delivery through various channels (e.g., SMTP).
 */

#ifndef HYDROGEN_CONFIG_NOTIFY_H
#define HYDROGEN_CONFIG_NOTIFY_H

#include <stdbool.h>
#include <jansson.h>
#include "config_forward.h"

// Default values
#define DEFAULT_SMTP_PORT 587
#define DEFAULT_SMTP_TLS true
#define DEFAULT_SMTP_TIMEOUT 30
#define DEFAULT_SMTP_MAX_RETRIES 3

// SMTP configuration structure
typedef struct SMTPConfig {
    char* host;           // SMTP server hostname
    int port;            // SMTP server port
    char* username;      // SMTP authentication username
    char* password;      // SMTP authentication password
    bool use_tls;        // Whether to use TLS
    int timeout;         // Connection timeout in seconds
    int max_retries;     // Maximum number of retry attempts
    char* from_address;  // Default from address
} SMTPConfig;

// Notify configuration structure
typedef struct NotifyConfig {
    bool enabled;        // Whether notification system is enabled
    char* notifier;      // Type of notifier (e.g., "SMTP")
    SMTPConfig smtp;     // SMTP configuration
} NotifyConfig;

/*
 * Load notification configuration from JSON
 *
 * This function loads the notification configuration from the provided JSON root,
 * applying any environment variable overrides and using secure defaults
 * where values are not specified.
 *
 * @param root JSON root object containing configuration
 * @param config Pointer to AppConfig structure to update
 * @return true if successful, false on error
 */
bool load_notify_config(json_t* root, AppConfig* config);

/*
 * Initialize notify configuration with default values
 *
 * This function initializes a new NotifyConfig structure with
 * default values that provide a secure baseline configuration.
 *
 * @param config Pointer to NotifyConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 * - If memory allocation fails
 */
int config_notify_init(NotifyConfig* config);

/*
 * Free resources allocated for notify configuration
 *
 * This function frees all resources allocated by config_notify_init.
 * It safely handles NULL pointers and partial initialization.
 *
 * @param config Pointer to NotifyConfig structure to cleanup
 */
void config_notify_cleanup(NotifyConfig* config);

/*
 * Validate notify configuration values
 *
 * This function performs validation of the configuration:
 * - Verifies enabled status
 * - Validates notifier type
 * - Checks SMTP configuration if enabled
 *
 * @param config Pointer to NotifyConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If notifier type is invalid
 * - If SMTP configuration is invalid when enabled
 */
int config_notify_validate(const NotifyConfig* config);

#endif /* HYDROGEN_CONFIG_NOTIFY_H */