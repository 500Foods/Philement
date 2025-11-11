/*
 * Launch WebServer Helper Functions
 * 
 * Header file for helper functions extracted from launch_webserver.c
 */

#ifndef LAUNCH_WEBSERVER_HELPERS_H
#define LAUNCH_WEBSERVER_HELPERS_H

#include <stdbool.h>

/*
 * Check if web server daemon is ready and log its status
 * 
 * This function encapsulates the repeated logic of checking if the webserver
 * daemon is bound to a port and logging detailed status information.
 * 
 * @return true if server is ready (daemon bound to port), false otherwise
 */
bool check_webserver_daemon_ready(void);

#endif // LAUNCH_WEBSERVER_HELPERS_H