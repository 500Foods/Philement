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
 * Dump notification configuration
 *
 * This function dumps the current state of the notification configuration
 * for debugging purposes.
 *
 * @param config Pointer to NotifyConfig structure to dump
 */
void dump_notify_config(const NotifyConfig* config);

/*
 * Free resources allocated for notify configuration
 *
 * This function frees all resources allocated during configuration loading.
 * It safely handles NULL pointers and partial initialization.
 *
 * @param config Pointer to NotifyConfig structure to cleanup
 */
void cleanup_notify_config(NotifyConfig* config);

#endif /* HYDROGEN_CONFIG_NOTIFY_H */
