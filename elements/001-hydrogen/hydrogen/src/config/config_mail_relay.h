/*
 * Mail Relay Configuration
 *
 * Defines the configuration structure for the mail relay subsystem.
 * This provides mail relay functionality with support for multiple
 * outbound SMTP servers and queue management.
 */

#ifndef CONFIG_MAIL_RELAY_H
#define CONFIG_MAIL_RELAY_H

#include <src/globals.h>

// System headers
#include <stdbool.h>

// Third-party headers
#include <jansson.h>

// Project headers
#include "config_forward.h"  // For AppConfig forward declaration

// Forward declarations from other modules
struct NotifyConfig;

// Mail relay event rule: event key to Lua script mapping
typedef struct {
    char* event_key;
    char* script_name;
} MailEventRule;

// Mail relay template settings
typedef struct {
    int ReloadIntervalSeconds;
} MailRelayTemplateSettings;

// Mail relay OTP / MFA settings
typedef struct {
    int Digits;            // Numeric code length (default 6)
    int ExpirySeconds;     // Code lifetime in seconds (default 300)
    int MaxAttempts;       // Verify attempts before lockout (default 5)
} MailRelayOtpSettings;

// Mail relay events configuration
typedef struct {
    bool Enabled;
    int MaxEventsPerInterval;  // Token-bucket capacity per event key
    int EventIntervalSeconds;  // Token-bucket refill interval
    MailEventRule Rules[MAX_MAIL_RELAY_EVENT_RULES];
    int RuleCount;
} MailRelayEvents;

// Mail relay test-only configuration (gated by SendRawOnLaunch)
typedef struct {
    bool SendRawOnLaunch;
    bool SendOtpOnLaunch;       // Launch-time OTP send + self-verify (blackbox coverage seam)
    bool FailNextSendOnLaunch;  // Launch-time forced transient SMTP failure (coverage seam)
    char* TestFrom;
    char* TestTo;
    char* TestSubject;
    char* TestBody;
} MailRelayTest;

void cleanup_mail_relay_test(MailRelayTest* test);

// Outbound server configuration
typedef struct OutboundServer {
    char* Host;                 // SMTP server hostname
    char* Port;                 // SMTP server port (string for env var support)
    char* Username;             // SMTP authentication username
    char* Password;             // SMTP authentication password
    bool UseTLS;                // Whether to use TLS (legacy boolean)
    int TLSMode;                // TLS mode: MAIL_TLS_MODE_*
    char* CAPath;               // Path to CA bundle
    int AuthMode;               // Auth mode: MAIL_AUTH_MODE_*
    int TimeoutSeconds;         // Connection and operation timeout
} OutboundServer;

// Queue settings configuration
typedef struct QueueSettings {
    int MaxQueueSize;           // Maximum number of messages in queue (legacy alias)
    int MaxInMemory;            // Max in-memory capacity before persistence
    bool Persist;               // Enable queue persistence to database
    int RetryAttempts;          // Number of retry attempts
    int RetryDelaySeconds;      // Initial delay between retries
    int InitialDelaySeconds;    // Delay before first attempt
    int MaxDelaySeconds;        // Maximum delay between retries
    int DebounceSeconds;        // Debounce window for event-generated mail
    int StaleTimeoutSeconds;    // How long a 'sending' row may remain before recovery
} QueueSettings;

// Main mail relay configuration structure
typedef struct MailRelayConfig {
    bool Enabled;                           // Whether mail relay is enabled
    bool OutboundEnabled;                   // Whether outbound sending is enabled
    bool InboundEnabled;                    // Whether inbound relay listener is enabled
    char* Database;                         // Database connection name for mail tables
    char* DefaultFrom;                      // Default from address
    char* DefaultReplyTo;                   // Default reply-to address
    char* AdminRecipients[MAX_MAIL_RELAY_ADMIN_RECIPIENTS]; // Admin notification addresses
    int AdminRecipientCount;                 // Number of configured admin recipients
    int ListenPort;                         // Port to listen on for incoming mail
    int Workers;                            // Number of worker threads

    // Queue configuration
    QueueSettings Queue;

    // Template configuration
    MailRelayTemplateSettings Templates;

    // OTP / MFA configuration
    MailRelayOtpSettings Otp;

    // Event configuration
    MailRelayEvents Events;

    // Test configuration (SendRawOnLaunch smoke test)
    MailRelayTest Test;

    // Outbound server configuration
    int OutboundServerCount;                            // Number of configured servers
    OutboundServer Servers[MAX_OUTBOUND_SERVERS];       // Array of server configs
} MailRelayConfig;

/*
 * Helper function to cleanup a single server configuration
 *
 * @param server Pointer to OutboundServer structure to cleanup
 */
void cleanup_server(OutboundServer* server);

/*
 * Helper function to cleanup mail relay events configuration
 *
 * @param events Pointer to MailRelayEvents structure to cleanup
 */
void cleanup_mail_relay_events(MailRelayEvents* events);

/*
 * Helper function to cleanup mail relay test configuration
 *
 * @param test Pointer to MailRelayTest structure to cleanup
 */
void cleanup_mail_relay_test(MailRelayTest* test);

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
 * in a structured format with proper indentation. Sensitive fields
 * are redacted.
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
