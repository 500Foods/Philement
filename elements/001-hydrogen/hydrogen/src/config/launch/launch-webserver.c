/*
 * Web Server Subsystem Launch
 * 
 * This module handles the launch of the web server subsystem.
 * It ensures that the web server is properly initialized and registered.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "launch.h"
#include "launch-webserver.h"
#include "../../logging/logging.h"
#include "../../config/config.h"
#include "../../state/registry/subsystem_registry.h"
#include "../../state/registry/subsystem_registry_integration.h"
#include "../../state/startup/startup_webserver.h"
#include "../../webserver/web_server.h"  // For shutdown_web_server()

// External declarations
extern AppConfig* app_config;
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t web_server_shutdown;

/**
 * Launch the webserver subsystem
 * 
 * This function launches the webserver subsystem by initializing
 * the webserver and registering it in the subsystem registry.
 * 
 * @return true if webserver was successfully launched, false otherwise
 */
bool launch_webserver_subsystem(void) {
    // Prevent initialization during any shutdown state
    if (server_stopping || web_server_shutdown) {
        log_this("WebServer", "Cannot launch webserver during shutdown", LOG_LEVEL_STATE);
        return false;
    }

    // Only proceed if we're in startup phase or running
    if (!server_starting && !server_running) {
        log_this("WebServer", "Cannot launch webserver outside startup or running phase", LOG_LEVEL_STATE);
        return false;
    }

    // Check if webserver is enabled in configuration
    if (!app_config->web.enabled) {
        log_this("WebServer", "Webserver is disabled in configuration", LOG_LEVEL_STATE);
        return false;
    }

    // The webserver subsystem is already registered in the registry by launch.c
    // We should just update its state to indicate it's ready to be started
    // The registry will handle the actual initialization when it's started
    // The webserver subsystem is already registered in the registry by launch.c
    // We should just update its state to indicate it's ready to be started
    // The registry will handle the actual initialization when it's started
    int subsys_id = get_subsystem_id_by_name("WebServer");
    if (subsys_id >= 0) {
        update_subsystem_state(subsys_id, SUBSYSTEM_INACTIVE);
        log_this("WebServer", "Webserver subsystem marked as inactive", LOG_LEVEL_STATE);
    }
    
    log_this("WebServer", "Webserver subsystem ready for launch", LOG_LEVEL_STATE);
    
    // Mark the subsystem as running in the registry
    update_subsystem_on_startup("WebServer", true);
    log_this("WebServer", "WebServer subsystem launched successfully", LOG_LEVEL_STATE);
    
    return true;
}

/**
 * Free resources allocated during webserver launch
 * 
 * This function frees any resources allocated during the webserver launch phase.
 * It should be called during the LANDING: WEBSERVER phase of the application.
 */
void free_webserver_resources(void) {
    // Free any resources allocated during webserver launch
    log_this("WebServer", "Freeing webserver resources", LOG_LEVEL_STATE);
    
    // Call the webserver cleanup function
    shutdown_web_server();
    
    log_this("WebServer", "Webserver resources freed", LOG_LEVEL_STATE);
    
    // Update the Webserver subsystem state to inactive
    int subsys_id = get_subsystem_id_by_name("WebServer");
    if (subsys_id >= 0) {
        update_subsystem_state(subsys_id, SUBSYSTEM_INACTIVE);
        log_this("WebServer", "Webserver subsystem marked as inactive", LOG_LEVEL_STATE);
    }
}