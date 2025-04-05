/*
 * Web Server Subsystem Launch Header
 * 
 * This header declares functions for launching and checking readiness
 * of the web server subsystem. It provides HTTP/REST API functionality
 * for configuration and control.
 * 
 * Dependencies:
 * - Network subsystem must be initialized and ready
 * - Thread subsystem must be initialized and ready
 */

#ifndef HYDROGEN_LAUNCH_WEBSERVER_H
#define HYDROGEN_LAUNCH_WEBSERVER_H

#include <stdbool.h>
#include "launch.h"

/*
 * Get webserver subsystem readiness status (cached)
 *
 * This function provides a cached version of the webserver readiness check.
 * It will return the cached result if available, or perform a new check
 * if no cached result exists.
 *
 * Memory Management:
 * - The returned LaunchReadiness structure and its messages are managed internally
 * - Do not free the returned structure or its messages
 * - Cache is automatically cleared when:
 *   - The webserver subsystem is launched
 *   - The system state changes
 *
 * @return LaunchReadiness structure with readiness status and messages (do not free)
 */
LaunchReadiness get_webserver_readiness(void);

/*
 * Check webserver subsystem launch readiness (direct check)
 * 
 * This function performs readiness checks for the webserver subsystem by:
 * - Verifying system state and dependencies (Threads, Network)
 * - Checking protocol configuration (IPv4/IPv6)
 * - Validating interface availability
 * - Checking port and web root configuration
 * 
 * Note: Prefer using get_webserver_readiness() instead of calling this directly
 * to avoid redundant checks and potential memory leaks.
 * 
 * @return LaunchReadiness structure with readiness status and messages
 */
LaunchReadiness check_webserver_launch_readiness(void);

/*
 * Launch the webserver subsystem
 * 
 * This function launches the webserver subsystem by:
 * 1. Verifying system state and configuration
 * 2. Initializing the HTTP server
 * 3. Creating and registering server threads
 * 4. Waiting for successful port binding
 * 5. Updating the subsystem registry
 * 
 * @return 1 on success, 0 on failure
 */
int launch_webserver_subsystem(void);

/**
 * Free resources allocated during webserver launch
 * 
 * This function frees any resources allocated during the webserver launch phase.
 * It should be called during the LANDING: WEBSERVER phase of the application.
 */
void free_webserver_resources(void);

/**
 * Check if web server is running
 * 
 * This function checks if the web server is currently running
 * and available to handle requests.
 * 
 * @return 1 if running, 0 if not running
 */
int is_web_server_running(void);

#endif /* HYDROGEN_LAUNCH_WEBSERVER_H */