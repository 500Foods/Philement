/*
 * Configuration Defaults Header
 *
 * Provides centralized default configuration values for the Hydrogen server.
 * This header declares functions to initialize app_config with secure baseline
 * values that can be overridden by JSON configuration files and environment variables.
 */

#ifndef HYDROGEN_CONFIG_DEFAULTS_H
#define HYDROGEN_CONFIG_DEFAULTS_H

#include "config_forward.h"  // For AppConfig forward declaration

/*
 * Initialize app_config with configuration defaults
 *
 * This function sets up the AppConfig structure with secure baseline defaults
 * for multiple configuration sections. The function centralizes all default
 * values in one place for easier maintenance and consistency.
 *
 * Currently implemented sections:
 * A. Server - Basic server identification and logging
 * B. Network - Network interface and port configuration
 * C. Database - Database connection settings
 * D. Logging - Logging destinations and levels
 * E. WebServer - HTTP server settings and paths
 * F. API - REST API configuration and JWT settings
 * G. Swagger - API documentation and UI configuration
 * H. WebSocket - WebSocket server and connection settings
 * I. Terminal - Terminal access and session management
 * J. mDNS Server - Service discovery server
 * K. mDNS Client - Service discovery client
 * L. Mail Relay - Email relay configuration
 * M. Print - Print server configuration
 * N. Resources - System resource limits and monitoring
 * O. OIDC - OpenID Connect authentication
 * P. Notify - Notification system
 *
 * @param config Pointer to the AppConfig structure to initialize
 * @return true on success, false on failure (memory allocation errors)
 */
bool initialize_config_defaults(AppConfig* config);

void initialize_config_defaults_server(AppConfig* config);
void initialize_config_defaults_network(AppConfig* config);
void initialize_config_defaults_database(AppConfig* config);
void initialize_config_defaults_logging(AppConfig* config);
void initialize_config_defaults_webserver(AppConfig* config);
void initialize_config_defaults_api(AppConfig* config);
void initialize_config_defaults_swagger(AppConfig* config);
void initialize_config_defaults_websocket(AppConfig* config);
void initialize_config_defaults_terminal(AppConfig* config);
void initialize_config_defaults_mdns_server(AppConfig* config);
void initialize_config_defaults_mdns_client(AppConfig* config);
void initialize_config_defaults_mail_relay(AppConfig* config);
void initialize_config_defaults_print(AppConfig* config);
void initialize_config_defaults_resources(AppConfig* config);
void initialize_config_defaults_oidc(AppConfig* config);
void initialize_config_defaults_notify(AppConfig* config);

#endif /* HYDROGEN_CONFIG_DEFAULTS_H */
