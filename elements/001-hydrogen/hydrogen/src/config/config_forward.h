/*
 * Forward declarations for configuration types
 */

#ifndef CONFIG_FORWARD_H
#define CONFIG_FORWARD_H

// Core configuration types
typedef struct AppConfig AppConfig;

// API and security types
typedef struct APIConfig APIConfig;

// Web and network types
typedef struct WebServerConfig WebServerConfig;
typedef struct WebSocketConfig WebSocketConfig;
typedef struct NetworkConfig NetworkConfig;

// System monitoring types
typedef struct MonitoringConfig MonitoringConfig;

// Motion types
typedef struct MotionConfig MotionConfig;

// Print queue types
typedef struct PrintQueueConfig PrintQueueConfig;
typedef struct PrintQueuePrioritiesConfig PrintQueuePrioritiesConfig;
typedef struct PrintQueueTimeoutsConfig PrintQueueTimeoutsConfig;
typedef struct PrintQueueBuffersConfig PrintQueueBuffersConfig;

// OIDC types
typedef struct OIDCConfig OIDCConfig;
typedef struct OIDCEndpointsConfig OIDCEndpointsConfig;
typedef struct OIDCKeysConfig OIDCKeysConfig;
typedef struct OIDCTokensConfig OIDCTokensConfig;
typedef struct OIDCSecurityConfig OIDCSecurityConfig;

// Logging types
typedef struct LoggingConfig LoggingConfig;
typedef struct LoggingConsoleConfig LoggingConsoleConfig;
typedef struct LoggingFileConfig LoggingFileConfig;
typedef struct LoggingDatabaseConfig LoggingDatabaseConfig;

// Resource types
typedef struct ResourceConfig ResourceConfig;

// MDNS types
typedef struct MDNSServerConfig MDNSServerConfig;

// Swagger types
typedef struct SwaggerConfig SwaggerConfig;

#endif /* CONFIG_FORWARD_H */