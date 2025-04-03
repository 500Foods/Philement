/*
 * Web Server Subsystem Launch Header
 * 
 * This header declares functions for launching the web server subsystem.
 */

#ifndef HYDROGEN_LAUNCH_WEBSERVER_H
#define HYDROGEN_LAUNCH_WEBSERVER_H

#include <stdbool.h>

/**
 * Launch the webserver subsystem
 * 
 * This function launches the webserver subsystem by setting up
 * the HTTP server and registering it in the subsystem registry.
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