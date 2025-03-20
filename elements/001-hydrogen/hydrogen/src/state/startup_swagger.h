/*
 * Swagger Subsystem Startup Handler Header
 * 
 * This header declares the initialization interface for the Swagger subsystem.
 * The Swagger system provides API documentation and interactive testing capabilities.
 * Note: Requires the web server to be initialized first as it serves the Swagger UI.
 */

#ifndef HYDROGEN_STARTUP_SWAGGER_H
#define HYDROGEN_STARTUP_SWAGGER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize Swagger subsystem
 * 
 * This function initializes the Swagger documentation system.
 * It provides API documentation capabilities:
 * - Interactive API documentation
 * - API endpoint testing interface
 * - OpenAPI specification hosting
 * - API schema validation
 * 
 * Dependencies:
 * - Requires web server to be running
 * - Needs proper route configuration
 * - Requires valid OpenAPI specifications
 * 
 * @return 1 on success, 0 on failure
 */
int init_swagger_subsystem(void);

/**
 * Shut down the Swagger subsystem
 * 
 * This function performs cleanup and shutdown of the Swagger documentation system.
 * It ensures proper resource release and termination of documentation services.
 * 
 * Actions performed:
 * - Unregister API routes
 * - Close documentation endpoints
 * - Free documentation resources
 * - Clean up temporary files
 */
void shutdown_swagger(void);

#ifdef __cplusplus
}
#endif

#endif /* HYDROGEN_STARTUP_SWAGGER_H */