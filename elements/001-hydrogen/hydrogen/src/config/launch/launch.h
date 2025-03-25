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

#include <stdbool.h>

// Result of a launch readiness check
typedef struct {
    const char* subsystem;  // Name of the subsystem
    bool ready;             // Is the subsystem ready to launch?
    const char** messages;  // Array of readiness messages (NULL-terminated)
} LaunchReadiness;

// Check if individual subsystems are ready to launch
LaunchReadiness check_logging_launch_readiness(void);
LaunchReadiness check_terminal_launch_readiness(void);
LaunchReadiness check_mdns_client_launch_readiness(void);
LaunchReadiness check_smtp_relay_launch_readiness(void);
LaunchReadiness check_swagger_launch_readiness(void);
LaunchReadiness check_webserver_launch_readiness(void);
LaunchReadiness check_websocket_launch_readiness(void);
LaunchReadiness check_print_launch_readiness(void);
LaunchReadiness check_payload_launch_readiness(void);
LaunchReadiness check_network_launch_readiness(void);

// Run all launch readiness checks in the correct order
// Returns true if all required subsystems are ready to launch
bool check_all_launch_readiness(void);

#endif /* LAUNCH_READINESS_H */