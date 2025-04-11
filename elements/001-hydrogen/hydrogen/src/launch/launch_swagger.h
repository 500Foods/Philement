/*
 * Launch Swagger Subsystem Header
 * 
 * This header declares the initialization interface for the Swagger subsystem.
 * The Swagger subsystem provides API documentation and testing interface.
 * 
 * Dependencies:
 * - API subsystem must be initialized and ready
 * - Payload subsystem must be initialized and ready (for serving Swagger files)
 */

#ifndef HYDROGEN_LAUNCH_SWAGGER_H
#define HYDROGEN_LAUNCH_SWAGGER_H

#include "launch.h"

/**
 * Check if the Swagger subsystem is ready to launch
 * 
 * Verifies:
 * 1. API subsystem is Go for Launch
 * 2. Payload subsystem is Go for Launch
 * 3. Swagger is enabled
 * 4. Swagger prefix is valid
 * 
 * @return LaunchReadiness struct with status and messages
 */
LaunchReadiness check_swagger_launch_readiness(void);

/**
 * Launch Swagger subsystem
 * 
 * Initializes the Swagger UI and registers its endpoints with the web server.
 * Requires both API and Payload subsystems to be running.
 * 
 * @return 1 on success, 0 on failure
 */
int launch_swagger_subsystem(void);

/**
 * Check if Swagger subsystem is running
 * 
 * @return 1 if running, 0 if not running
 */
int is_swagger_running(void);

#endif /* HYDROGEN_LAUNCH_SWAGGER_H */