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
 * - Threads
 * - Network
 * - Database
 * - Logging
 * - WebServer
 * - API
 * - Swagger
 * - WebSockets
 * - Terminal
 * - mDNS Server
 * - mDNS Client
 * - MailRelay
 * - Print
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
#include "../threads/threads.h"
#include "../state/state_types.h"  // For SubsystemState and LaunchReadiness
#include "launch_registry.h"       // Registry must be first

// Core launch functions
ReadinessResults handle_readiness_checks(void);
bool check_all_launch_readiness(void);
bool handle_launch_plan(const ReadinessResults* results);
void handle_launch_review(const ReadinessResults* results);

// Main system startup function
int startup_hydrogen(const char* config_path);

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

// Subsystem registration functions
void register_api(void);
void register_swagger(void);

// Subsystem launch functions (in standard order)
int launch_registry_subsystem(void);  // Must be first
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