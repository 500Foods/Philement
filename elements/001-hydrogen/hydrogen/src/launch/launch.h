/*
 * Launch System Header
 * 
 * DESIGN PRINCIPLES:
 * - All subsystems are equal in importance
 * - No subsystem has inherent priority over others
 * - Dependencies determine what's needed, not importance
 * - The processing order below is for consistency only
 * 
 * This module defines the interfaces for the launch system and its subsystems.
 * All subsystem initialization functions follow the pattern launch_*_subsystem
 * and return 1 on success, 0 on failure.
 * 
 * Standard Processing Order (for consistency, not priority):
 * - 01. Registry (must be first, handles subsystem registration)
 * - 02. Payload
 * - 03. Threads
 * - 04. Network
 * - 05. Database
 * - 06. Logging
 * - 07. WebServer
 * - 08. API
 * - 09. Swagger
 * - 10. WebSockets
 * - 11. Terminal
 * - 12. mDNS Server
 * - 13. mDNS Client
 * - 14. MailRelay
 * - 15. Print
 * 
 * Each subsystem:
 * - Determines its own readiness independently
 * - Manages its own initialization
 * - Launches when its dependencies are met
 * - Is equally important in the system
 */

#ifndef LAUNCH_H
#define LAUNCH_H

// System includes
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>

// Project includes
#include "../config/config.h"
#include "../threads/threads.h"
#include "../state/state_types.h"  // For SubsystemState and LaunchReadiness
#include "launch_registry.h"       // Registry must be first

extern AppConfig *app_config;

// External system state flags
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_running;

// Core launch functions
ReadinessResults handle_readiness_checks(void);
bool check_all_launch_readiness(void);
bool handle_launch_plan(const ReadinessResults* results);
void handle_launch_review(const ReadinessResults* results);

// Main system startup function
int startup_hydrogen(const char* config_path);

// Utility functions
char* get_current_config_path(void);
LaunchReadiness get_api_readiness(void);
int is_api_running(void);

// Subsystem registration functions
void register_api(void);
void register_swagger(void);

void free_threads_resources(void);
void free_payload_resources(void);

// External declarations
extern volatile sig_atomic_t mail_relay_system_shutdown;
extern volatile sig_atomic_t mdns_client_system_shutdown;
extern volatile sig_atomic_t mdns_server_system_shutdown;
extern volatile sig_atomic_t web_server_shutdown;
extern volatile sig_atomic_t network_system_shutdown;

LaunchReadiness get_network_readiness(void);

// Subsystem readiness checks (in standard order)
LaunchReadiness check_registry_launch_readiness(void);  // Must be first
LaunchReadiness check_payload_launch_readiness(void);
LaunchReadiness check_threads_launch_readiness(void);
LaunchReadiness check_network_launch_readiness(void);
LaunchReadiness check_database_launch_readiness(void);
LaunchReadiness check_logging_launch_readiness(void);
LaunchReadiness check_webserver_launch_readiness(void);
LaunchReadiness check_api_launch_readiness(void);
LaunchReadiness check_swagger_launch_readiness(void);
LaunchReadiness check_websocket_launch_readiness(void);
LaunchReadiness check_terminal_launch_readiness(void);
LaunchReadiness check_mdns_server_launch_readiness(void);
LaunchReadiness check_mdns_client_launch_readiness(void);
LaunchReadiness check_mail_relay_launch_readiness(void);
LaunchReadiness check_print_launch_readiness(void);
LaunchReadiness check_notify_launch_readiness(void);
LaunchReadiness check_oidc_launch_readiness(void);

// Subsystem launch functions 
int launch_registry_subsystem(bool is_restart);  // Must be first
int launch_payload_subsystem(void);
int launch_threads_subsystem(void);
int launch_network_subsystem(void);
int launch_notify_subsystem(void);
int launch_database_subsystem(void);
int launch_logging_subsystem(void);
int launch_webserver_subsystem(void);
int launch_api_subsystem(void);
int launch_swagger_subsystem(void);
int launch_websocket_subsystem(void);
int launch_terminal_subsystem(void);
int launch_mdns_server_subsystem(void);
int launch_mdns_client_subsystem(void);
int launch_mail_relay_subsystem(void);
int launch_print_subsystem(void);
int launch_oidc_subsystem(void);

int is_api_running(void);

void shutdown_database_subsystem(void);
void shutdown_network_subsystem(void);
void shutdown_logging_subsystem(void);
void shutdown_notify_subsystem(void);
void shutdown_web_server(void);
void shutdown_api(void);
void shutdown_swagger(void);
void shutdown_terminal(void);
void shutdown_mdns_server(void);
void shutdown_mdns_client(void);
void shutdown_mail_relay(void);
void shutdown_print_queue(void);
void stop_websocket_server(void);

// Notification configuration limits
#define MIN_SMTP_PORT 1
#define MAX_SMTP_PORT 65535
#define MIN_SMTP_TIMEOUT 1
#define MAX_SMTP_TIMEOUT 300
#define MIN_SMTP_RETRIES 0
#define MAX_SMTP_RETRIES 10

// OIDC configuration limits
#define MIN_OIDC_PORT 1024
#define MAX_OIDC_PORT 65535
#define MIN_TOKEN_LIFETIME 300        // 5 minutes
#define MAX_TOKEN_LIFETIME 86400      // 24 hours
#define MIN_REFRESH_LIFETIME 3600     // 1 hour
#define MAX_REFRESH_LIFETIME 2592000  // 30 days
#define MIN_KEY_ROTATION_DAYS 1
#define MAX_KEY_ROTATION_DAYS 90

#endif /* LAUNCH_H */
