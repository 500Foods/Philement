/*
 * Launch Readiness Subsystem
 * 
 * This module manages pre-launch checks to ensure subsystem dependencies
 * are met before attempting to start each component.
 * 
 * The launch readiness system evaluates each subsystem's prerequisites
 * and determines whether it's safe to proceed with initialization.
 */

#ifndef LAUNCH_READINESS_H
#define LAUNCH_READINESS_H

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

// Perform readiness checks on all subsystems
ReadinessResults handle_readiness_checks(void);

// Coordinate the overall launch sequence
bool check_all_launch_readiness(void);

// Execute the launch plan and make Go/No-Go decisions
bool handle_launch_plan(const ReadinessResults* results);

// Review and report final launch status
void handle_launch_review(const ReadinessResults* results, time_t start_time);

// Check if individual subsystems are ready to launch
LaunchReadiness check_logging_launch_readiness(void);
LaunchReadiness check_database_launch_readiness(void);
LaunchReadiness check_terminal_launch_readiness(void);
LaunchReadiness check_mdns_server_launch_readiness(void);
LaunchReadiness check_mdns_client_launch_readiness(void);
LaunchReadiness check_mail_relay_launch_readiness(void);
LaunchReadiness check_swagger_launch_readiness(void);
LaunchReadiness check_webserver_launch_readiness(void);
LaunchReadiness check_websocket_launch_readiness(void);
LaunchReadiness check_print_launch_readiness(void);
LaunchReadiness check_payload_launch_readiness(void);
LaunchReadiness check_threads_launch_readiness(void);
LaunchReadiness check_network_launch_readiness(void);
LaunchReadiness check_api_launch_readiness(void);


// Subsystem initialization and shutdown functions
void free_payload_resources(void);
int launch_threads_subsystem(void);  // Returns 1 on success, 0 on failure
void free_threads_resources(void);

// Main system startup function
int startup_hydrogen(const char* config_path);

// Network subsystem
int init_network_subsystem(void);
void shutdown_network_subsystem(void);

// Web server subsystem
int init_webserver_subsystem(void);
void shutdown_web_server(void);

// WebSocket subsystem
int init_websocket_subsystem(void);
void stop_websocket_server(void);

// Terminal subsystem
int init_terminal_subsystem(void);
void shutdown_terminal(void);

// mDNS subsystems
int init_mdns_server_subsystem(void);
void shutdown_mdns_server(void);
int init_mdns_client_subsystem(void);
void shutdown_mdns_client(void);

// Mail relay subsystem
int init_mail_relay_subsystem(void);
void shutdown_mail_relay(void);

// Swagger subsystem
int init_swagger_subsystem(void);
void shutdown_swagger(void);

// Print subsystem
int init_print_subsystem(void);
void shutdown_print_queue(void);

// Launch functions for individual subsystems
bool launch_payload_subsystem(void);
int launch_webserver_subsystem(void);

#endif /* LAUNCH_READINESS_H */