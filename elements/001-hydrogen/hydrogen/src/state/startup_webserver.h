/*
 * Web Server Subsystem Startup Handler Header
 * 
 * This header declares the initialization interface for the web server subsystem.
 * The web server is completely independent from the WebSocket server to:
 * 1. Allow independent scaling
 * 2. Enhance reliability through isolation
 * 3. Support flexible deployment
 * 4. Enable different security policies
 */

#ifndef HYDROGEN_STARTUP_WEBSERVER_H
#define HYDROGEN_STARTUP_WEBSERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize web server subsystem
 * 
 * This function initializes the HTTP/REST API server and starts
 * the web server thread. It is completely independent from
 * the WebSocket server.
 * 
 * @return 1 on success, 0 on failure
 */
int init_webserver_subsystem(void);

/**
 * Check if web server is running
 * 
 * This function checks if the web server is currently running
 * and available to handle requests.
 * 
 * @return 1 if running, 0 if not running
 */
int is_web_server_running(void);

#ifdef __cplusplus
}
#endif

#endif /* HYDROGEN_STARTUP_WEBSERVER_H */