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
 * This function launches the webserver subsystem by initializing
 * the webserver and registering it in the subsystem registry.
 * 
 * @return true if webserver was successfully launched, false otherwise
 */
bool launch_webserver_subsystem(void);

/**
 * Free resources allocated during webserver launch
 * 
 * This function frees any resources allocated during the webserver launch phase.
 * It should be called during the LANDING: WEBSERVER phase of the application.
 */
void free_webserver_resources(void);

#endif /* HYDROGEN_LAUNCH_WEBSERVER_H */