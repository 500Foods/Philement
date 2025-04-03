/*
 * Launch API Subsystem Header
 * 
 * This header declares the initialization interface for the API subsystem.
 * The API subsystem provides REST endpoints and depends on the web server
 * being initialized and ready.
 */

#ifndef HYDROGEN_LAUNCH_API_H
#define HYDROGEN_LAUNCH_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include "launch.h"

/**
 * Check if the API subsystem is ready to launch
 * 
 * Verifies:
 * 1. WebServer is Go for Launch
 * 2. API is enabled (defaults to true)
 * 3. API prefix is valid (defaults to "/api")
 * 
 * @return LaunchReadiness struct with status and messages
 */
LaunchReadiness check_api_launch_readiness(void);

/**
 * Launch API subsystem
 * 
 * Initializes the API endpoints and registers them with the web server.
 * Requires the web server to be running.
 * 
 * @return 1 on success, 0 on failure
 */
int launch_api_subsystem(void);

/**
 * Check if API subsystem is running
 * 
 * @return 1 if running, 0 if not running
 */
int is_api_running(void);

/**
 * Shutdown API subsystem
 * 
 * Cleans up API resources during system shutdown.
 * This function is called during the landing phase.
 */
void shutdown_api(void);

#ifdef __cplusplus
}
#endif

#endif /* HYDROGEN_LAUNCH_API_H */