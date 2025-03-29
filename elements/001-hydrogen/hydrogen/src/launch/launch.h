/*
 * Launch System Header
 * 
 * This module defines the interfaces for the launch system and its subsystems.
 * All subsystem initialization functions follow the pattern launch_*_subsystem
 * and return 1 on success, 0 on failure.
 * 
 * Subsystems are organized in the following standard order:
 * 1. Subsystem Registry
 * 2. Payload
 * 3. Threads
 * 4. Network
 * 5. Database
 * 6. Logging
 * 7. WebServer
 * 8. API
 * 9. Swagger
 * 10. WebSockets
 * 11. Terminal
 * 12. mDNS Server
 * 13. mDNS Client
 * 14. MailRelay
 * 15. Print
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
#include "../state/registry/subsystem_registry.h"
#include "../utils/utils_threads.h"

// Result of a launch readiness check
typedef struct {
    const char* subsystem;  // Name of the subsystem
    bool ready;             // Is the subsystem ready to launch?
    const char** messages;  // Array of readiness messages (NULL-terminated)
} LaunchReadiness;

// Structure to track launch status for each subsystem
typedef struct {
    const char* subsystem;    // Subsystem name
    bool ready;              // Ready status from readiness check
    SubsystemState state;    // Current state in registry
    time_t launch_time;      // When launch started
} LaunchStatus;

// Structure to hold readiness check results
typedef struct {
    struct {
        const char* subsystem;
        bool ready;
    } results[15];  // Must match number of subsystems
    size_t total_checked;
    size_t total_ready;
    size_t total_not_ready;
    bool any_ready;
} ReadinessResults;

// Core launch functions
ReadinessResults handle_readiness_checks(void);
bool check_all_launch_readiness(void);
bool handle_launch_plan(const ReadinessResults* results);
void handle_launch_review(const ReadinessResults* results, time_t start_time);

// Main system startup function
int startup_hydrogen(const char* config_path);

// Subsystem readiness checks (in standard order)
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

// Subsystem launch functions (in standard order)
int launch_payload_subsystem(void);
void free_payload_resources(void);

int launch_threads_subsystem(void);
void free_threads_resources(void);

int launch_network_subsystem(void);
void shutdown_network_subsystem(void);

int launch_database_subsystem(void);
void shutdown_database_subsystem(void);

int launch_logging_subsystem(void);
void shutdown_logging_subsystem(void);

int launch_webserver_subsystem(void);
void shutdown_web_server(void);

int launch_api_subsystem(void);
void shutdown_api(void);

int launch_swagger_subsystem(void);
void shutdown_swagger(void);

int launch_websocket_subsystem(void);
void stop_websocket_server(void);

int launch_terminal_subsystem(void);
void shutdown_terminal(void);

int launch_mdns_server_subsystem(void);
void shutdown_mdns_server(void);

int launch_mdns_client_subsystem(void);
void shutdown_mdns_client(void);

int launch_mail_relay_subsystem(void);
void shutdown_mail_relay(void);

int launch_print_subsystem(void);
void shutdown_print_queue(void);

#endif /* LAUNCH_H */