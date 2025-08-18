/*
 * Landing Readiness System Header
 * 
 * DESIGN PRINCIPLES:
 * - All subsystems are equal in importance
 * - No subsystem has inherent priority over others
 * - Dependencies determine what's needed, not importance
 * - The processing order is reverse of launch for consistency
 * 
 * This module defines the interfaces for the landing readiness system.
 * All subsystem landing readiness functions follow the pattern check_*_landing_readiness
 * and return a LaunchReadiness struct.
 * 
 * Standard Processing Order (reverse of launch for consistency):
 * - Print
 * - MailRelay
 * - mDNS Client
 * - mDNS Server
 * - Terminal
 * - WebSockets
 * - Swagger
 * - API
 * - WebServer
 * - Database
 * - Logging
 * - Network
 * - Payload
 * - Threads
 * - Registry
 * 
 * Each subsystem:
 * - Determines its own readiness independently
 * - Manages its own landing sequence
 * - Lands when its dependents are inactive
 * - Is equally important in the system
 */

#ifndef LANDING_READINESS_H
#define LANDING_READINESS_H

// System includes
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// Project includes
#include "../state/state_types.h"  // For shared types

// Core landing readiness functions
ReadinessResults handle_landing_readiness(void);
bool handle_landing_plan(const ReadinessResults* results);
void handle_landing_review(const ReadinessResults* results, time_t start_time);

// Subsystem readiness checks (in reverse launch order)
LaunchReadiness check_print_landing_readiness(void);
LaunchReadiness check_mail_relay_landing_readiness(void);
LaunchReadiness check_mdns_client_landing_readiness(void);
LaunchReadiness check_mdns_server_landing_readiness(void);
LaunchReadiness check_terminal_landing_readiness(void);
LaunchReadiness check_websocket_landing_readiness(void);
LaunchReadiness check_swagger_landing_readiness(void);
LaunchReadiness check_api_landing_readiness(void);
LaunchReadiness check_webserver_landing_readiness(void);
LaunchReadiness check_database_landing_readiness(void);
LaunchReadiness check_logging_landing_readiness(void);
LaunchReadiness check_network_landing_readiness(void);
LaunchReadiness check_payload_landing_readiness(void);
LaunchReadiness check_threads_landing_readiness(void);
LaunchReadiness check_registry_landing_readiness(void);

// Memory management is handled by state_types.h

#endif /* LANDING_READINESS_H */
