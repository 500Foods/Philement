/*
 * Mail Relay Configuration
 *
 * Defines the configuration structure for the mail relay subsystem.
 * This provides mail relay functionality with support for multiple
 * outbound SMTP servers and queue management.
 */

#ifndef CONFIG_MAIL_RELAY_H
#define CONFIG_MAIL_RELAY_H

#include "../globals.h"

// System headers
#include <stdbool.h>

// Third-party headers
#include <jansson.h>

// Project headers
#include "config_forward.h"  // For AppConfig forward declaration

// Forward declarations from other modules
struct NotifyConfig;

// Outbound server configuration
typedef struct OutboundServer {
    char* Host;          // SMTP server hostname
    char* Port;         // SMTP server port (string for env var support)
    char* Username;     // SMTP authentication username
    char* Password;     // SMTP authentication password
    bool UseTLS;        // Whether to use TLS
} OutboundServer;

// Queue settings configuration
typedef struct QueueSettings {
    int MaxQueueSize;       // Maximum number of messages in queue
    int RetryAttempts;      // Number of retry attempts
    int RetryDelaySeconds;  // Delay between retries
} QueueSettings;

// Main mail relay configuration structure
typedef struct MailRelayConfig {
    bool Enabled;           // Whether mail relay is enabled
    int ListenPort;         // Port to listen on for incoming mail
    int Workers;            // Number of worker threads
    
    // Queue configuration
    QueueSettings Queue;
    
    // Outbound server configuration
    int OutboundServerCount;                        // Number of configured servers
    OutboundServer Servers[MAX_OUTBOUND_SERVERS];   // Array of server configs
} MailRelayConfig;

/*
 * Helper function to cleanup a single server configuration
 *
 * @param server Pointer to OutboundServer structure to cleanup
 */
void cleanup_server(OutboundServer* server);

/*
 * Load mail relay configuration from JSON
 *
 * This function loads the mail relay configuration from the provided JSON root,
 * applying any environment variable overrides and using secure defaults
 * where values are not specified.
 *
 * @param root JSON root object containing configuration
 * @param config Pointer to AppConfig structure to update
 * @return true if successful, false on error
 */
bool load_mailrelay_config(json_t* root, AppConfig* config);

/*
 * Dump mail relay configuration for debugging
 *
 * This function outputs the current mail relay configuration settings
 * in a structured format with proper indentation.
 *
 * @param config Pointer to MailRelayConfig structure to dump
 */
void dump_mailrelay_config(const MailRelayConfig* config);

/*
 * Clean up mail relay configuration
 *
 * This function frees all resources allocated for the mail relay configuration.
 * It safely handles NULL pointers and partial initialization.
 * After cleanup, the structure is zeroed to prevent use-after-free.
 *
 * @param config Pointer to MailRelayConfig structure to cleanup
 */
void cleanup_mailrelay_config(MailRelayConfig* config);

#endif /* CONFIG_MAIL_RELAY_H */
