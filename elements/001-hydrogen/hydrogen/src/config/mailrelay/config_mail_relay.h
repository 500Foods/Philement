/*
 * Mail Relay Configuration
 *
 * Defines the configuration structure for the mail relay subsystem.
 * This provides mail relay functionality with support for multiple
 * outbound SMTP servers and queue management.
 */

#ifndef CONFIG_MAIL_RELAY_H
#define CONFIG_MAIL_RELAY_H

#include <stdbool.h>

// Maximum number of outbound servers
#define MAX_OUTBOUND_SERVERS 5

// Forward declarations from other modules
struct NotifyConfig;

// Default values for mail relay
#define DEFAULT_MAILRELAY_ENABLED true
#define DEFAULT_MAILRELAY_LISTEN_PORT 587
#define DEFAULT_MAILRELAY_WORKERS 2

// Default values for queue settings
#define DEFAULT_MAILRELAY_MAX_QUEUE_SIZE 1000
#define DEFAULT_MAILRELAY_RETRY_ATTEMPTS 3
#define DEFAULT_MAILRELAY_RETRY_DELAY 300  // seconds

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
 * Initialize mail relay configuration with default values
 *
 * This function initializes a new MailRelayConfig structure with
 * default values that provide a secure baseline configuration.
 *
 * @param config Pointer to MailRelayConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 * - If memory allocation fails
 */
int config_mailrelay_init(MailRelayConfig* config);

/*
 * Free resources allocated for mail relay configuration
 *
 * This function frees all resources allocated by config_mailrelay_init.
 * It safely handles NULL pointers and partial initialization.
 *
 * @param config Pointer to MailRelayConfig structure to cleanup
 */
void config_mailrelay_cleanup(MailRelayConfig* config);

/*
 * Validate mail relay configuration values
 *
 * This function performs validation of the configuration:
 * - Verifies enabled status and port ranges
 * - Validates worker count
 * - Checks queue settings
 * - Validates outbound server configurations
 *
 * @param config Pointer to MailRelayConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If enabled but no outbound servers configured
 * - If queue settings are invalid
 * - If any outbound server configuration is invalid
 */
int config_mailrelay_validate(const MailRelayConfig* config);

#endif /* CONFIG_MAIL_RELAY_H */