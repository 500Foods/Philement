/*
 * Forward declarations for configuration types
 *
 * This header provides forward declarations for all configuration structures
 * to avoid circular dependencies. The actual structure definitions are in
 * their respective headers.
 *
 * Sections are organized in A-O order:
 * A. Server
 * B. Network
 * C. Database
 * D. Logging
 * E. WebServer
 * F. API
 * G. Swagger
 * H. WebSocket
 * I. Terminal
 * J. mDNS Server
 * K. mDNS Client
 * L. Mail Relay
 * M. Print
 * N. Resources
 * O. OIDC
 * P. Notify
 */

#ifndef CONFIG_FORWARD_H
#define CONFIG_FORWARD_H

// Main application configuration structure
struct AppConfig;
typedef struct AppConfig AppConfig;

// A. Server Configuration
struct ServerConfig;
typedef struct ServerConfig ServerConfig;

// B. Network Configuration
struct NetworkConfig;
typedef struct NetworkConfig NetworkConfig;

// C. Database Configuration
struct DatabaseConfig;
typedef struct DatabaseConfig DatabaseConfig;

// D. Logging Configuration
struct LoggingConfig;
typedef struct LoggingConfig LoggingConfig;

struct LoggingConsoleConfig;
typedef struct LoggingConsoleConfig LoggingConsoleConfig;

struct LoggingDatabaseConfig;
typedef struct LoggingDatabaseConfig LoggingDatabaseConfig;

struct LoggingFileConfig;
typedef struct LoggingFileConfig LoggingFileConfig;

struct LoggingNotifyConfig;
typedef struct LoggingNotifyConfig LoggingNotifyConfig;

// E. WebServer Configuration
struct WebServerConfig;
typedef struct WebServerConfig WebServerConfig;

// F. API Configuration
struct APIConfig;
typedef struct APIConfig APIConfig;

// G. Swagger Configuration
struct SwaggerConfig;
typedef struct SwaggerConfig SwaggerConfig;

// H. WebSocket Configuration
struct WebSocketConfig;
typedef struct WebSocketConfig WebSocketConfig;

// I. Terminal Configuration
struct TerminalConfig;
typedef struct TerminalConfig TerminalConfig;

// J. mDNS Server Configuration
struct MDNSServerConfig;
typedef struct MDNSServerConfig MDNSServerConfig;

// K. mDNS Client Configuration
struct MDNSClientConfig;
typedef struct MDNSClientConfig MDNSClientConfig;

// L. Mail Relay Configuration (uses notify infrastructure)
struct MailRelayConfig;
typedef struct MailRelayConfig MailRelayConfig;

struct SMTPConfig;
typedef struct SMTPConfig SMTPConfig;

// M. Print Configuration
struct PrintQueueConfig;
typedef struct PrintQueueConfig PrintQueueConfig;

struct PrintQueuePrioritiesConfig;
typedef struct PrintQueuePrioritiesConfig PrintQueuePrioritiesConfig;

struct PrintQueueTimeoutsConfig;
typedef struct PrintQueueTimeoutsConfig PrintQueueTimeoutsConfig;

struct PrintQueueBuffersConfig;
typedef struct PrintQueueBuffersConfig PrintQueueBuffersConfig;

// N. Resources Configuration
struct ResourceConfig;
typedef struct ResourceConfig ResourceConfig;

// O. OIDC Configuration
struct OIDCConfig;
typedef struct OIDCConfig OIDCConfig;

struct OIDCEndpointsConfig;
typedef struct OIDCEndpointsConfig OIDCEndpointsConfig;

struct OIDCKeysConfig;
typedef struct OIDCKeysConfig OIDCKeysConfig;

struct OIDCTokensConfig;
typedef struct OIDCTokensConfig OIDCTokensConfig;

struct OIDCSecurityConfig;
typedef struct OIDCSecurityConfig OIDCSecurityConfig;

// P. Notify Configuration
struct NotifyConfig;
typedef struct NotifyConfig NotifyConfig;

#endif /* CONFIG_FORWARD_H */
