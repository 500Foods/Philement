/*
 * Subsystem Registry Integration
 *
 * This module integrates the subsystem registry with the Hydrogen server's
 * startup and shutdown processes. It registers all standard subsystems
 * and manages their dependencies.
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// System includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Project includes
#include "../state.h"
#include "subsystem_registry.h"
#include "../../logging/logging.h"
#include "../../utils/utils.h"
#include "../../utils/utils_threads.h"

// Subsystem init/shutdown declarations
extern int init_logging_subsystem(void);
extern void shutdown_logging_subsystem(void);

extern int init_webserver_subsystem(void);
extern void shutdown_web_server(void);

extern int init_websocket_subsystem(void);
extern void stop_websocket_server(void);

extern int init_mdns_server_subsystem(void);
extern void mdns_server_shutdown(mdns_server_t* server);

extern int init_mdns_client_subsystem(void);
extern void shutdown_mdns_client(void);

extern int init_smtp_relay_subsystem(void);
extern void shutdown_smtp_relay(void);

extern int init_swagger_subsystem(void);
extern void shutdown_swagger(void);

extern int init_terminal_subsystem(void);
extern void shutdown_terminal(void);

extern int init_print_subsystem(void);
extern void shutdown_print_queue(void);

// Forward declarations of static functions
static bool stop_subsystem_and_dependents(int subsystem_id);

/*
 * Update the registry after a subsystem has been shut down.
 * This should be called after a subsystem's shutdown function has completed.
 */
void update_subsystem_after_shutdown(const char* subsystem_name) {
    int id = get_subsystem_id_by_name(subsystem_name);
    if (id >= 0) {
        update_subsystem_state(id, SUBSYSTEM_INACTIVE);
    }
}


/*
 * Register a single subsystem based on its launch readiness result.
 * This is called during the Launch Go/No-Go process for subsystems that pass.
 * Returns the subsystem ID or -1 if registration failed.
 */
int register_subsystem_from_launch(const char* name, ServiceThreads* threads, 
                                   pthread_t* main_thread, volatile sig_atomic_t* shutdown_flag,
                                   int (*init_function)(void), void (*shutdown_function)(void)) {
    
    // Register the subsystem - no need for additional logging as the "Decide" line
    // in the launch readiness output already indicates the subsystem's status
    int subsys_id = register_subsystem(name, threads, main_thread, shutdown_flag, 
                                      init_function, shutdown_function);
    
    // Only log errors, as successful registration is already indicated by the "Decide" line
    if (subsys_id < 0) {
        log_this("Launch", "Failed to register subsystem '%s'", LOG_LEVEL_ERROR, name);
    }
    
    return subsys_id;
}

/*
 * Add a dependency for a subsystem from the launch process.
 * This is called during the Launch Go/No-Go process for each dependency identified.
 */
bool add_dependency_from_launch(int subsystem_id, const char* dependency_name) {
    const char* subsystem_name = NULL;
    
    // Get the subsystem name for logging
    if (subsystem_id >= 0 && subsystem_id < subsystem_registry.count) {
        subsystem_name = subsystem_registry.subsystems[subsystem_id].name;
    }
    
    // Add the dependency
    bool result = add_subsystem_dependency(subsystem_id, dependency_name);
    
    if (result) {
        log_this("Launch", "  Added dependency '%s' to subsystem '%s'", LOG_LEVEL_STATE, 
                dependency_name, subsystem_name ? subsystem_name : "Unknown");
    } else {
        log_this("Launch", "  Failed to add dependency '%s' to subsystem", LOG_LEVEL_ERROR, 
                dependency_name);
    }
    
    return result;
}

/*
 * Initialize the registry subsystem.
 * This initializes the registry itself as the first subsystem.
 */
void initialize_registry_subsystem(void) {
    // Initialize the registry
    init_subsystem_registry();
}

/*
 * Update the registry when a subsystem is started during the startup sequence.
 * This should be called after a subsystem's init function has been called.
 */
void update_subsystem_on_startup(const char* subsystem_name, bool success) {
    int id = get_subsystem_id_by_name(subsystem_name);
    if (id >= 0) {
        if (success) {
            update_subsystem_state(id, SUBSYSTEM_RUNNING);
        } else {
            update_subsystem_state(id, SUBSYSTEM_ERROR);
        }
    }
}

/*
 * Update the registry with all subsystems that were started during startup.
 * This synchronizes the registry with the actual state of the system.
 */
void update_subsystem_registry_on_startup(void) {
    // Check each subsystem for evidence it's running
    
    // Logging - Always starts first
    update_service_thread_metrics(&logging_threads);
    update_subsystem_on_startup("Logging", logging_threads.thread_count > 0);
    
    // Web Server
    update_service_thread_metrics(&web_threads);
    update_subsystem_on_startup("WebServer", web_threads.thread_count > 0);
    
    // WebSocket
    update_service_thread_metrics(&websocket_threads);
    update_subsystem_on_startup("WebSocket", websocket_threads.thread_count > 0);
    
    // mDNS Server
    update_service_thread_metrics(&mdns_server_threads);
    update_subsystem_on_startup("MDNSServer", mdns_server_threads.thread_count > 0);
    
    // mDNS Client - No thread, check if not shutdown
    update_subsystem_on_startup("MDNSClient", 
                              app_config && !mdns_client_system_shutdown);
    
    // SMTP Relay - No thread, check if not shutdown
    update_subsystem_on_startup("SMTPRelay", 
                              app_config && !smtp_relay_system_shutdown);
    
    // Swagger - No thread, check if not shutdown
    update_subsystem_on_startup("Swagger", 
                              app_config && !swagger_system_shutdown);
    
    // Terminal - No thread, check if not shutdown
    update_subsystem_on_startup("Terminal", 
                              app_config && !terminal_system_shutdown);
    
    // Print Queue
    update_service_thread_metrics(&print_threads);
    update_subsystem_on_startup("PrintQueue", print_threads.thread_count > 0);
}

/*
 * Update the registry when a subsystem is stopping during shutdown.
 * This should be called before a subsystem's shutdown function is called.
 */
void update_subsystem_on_shutdown(const char* subsystem_name) {
    int id = get_subsystem_id_by_name(subsystem_name);
    if (id >= 0) {
        update_subsystem_state(id, SUBSYSTEM_STOPPING);
    }
}

/*
 * Stop a subsystem and all its dependents safely.
 * Returns true if successful, false if any shutdown failed.
 */
static bool stop_subsystem_and_dependents(int subsystem_id) {
    SubsystemInfo* subsystem = &subsystem_registry.subsystems[subsystem_id];
    bool success = true;
    
    // Lock the registry while checking dependencies
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    // First check if any other running subsystems depend on this one
    for (int i = 0; i < subsystem_registry.count; i++) {
        SubsystemInfo* other = &subsystem_registry.subsystems[i];
        if (other->state == SUBSYSTEM_RUNNING) {
            for (int j = 0; j < other->dependency_count; j++) {
                if (strcmp(other->dependencies[j], subsystem->name) == 0) {
                    // Found a dependent - stop it first
                    pthread_mutex_unlock(&subsystem_registry.mutex);
                    success &= stop_subsystem_and_dependents(i);
                    pthread_mutex_lock(&subsystem_registry.mutex);
                }
            }
        }
    }
    
    // Now safe to stop this subsystem
    if (subsystem->state == SUBSYSTEM_RUNNING) {
        log_this("Shutdown", "Stopping subsystem '%s'", LOG_LEVEL_STATE, subsystem->name);
        
        // Mark as stopping
        update_subsystem_state(subsystem_id, SUBSYSTEM_STOPPING);
        
        // Release lock before calling shutdown function
        pthread_mutex_unlock(&subsystem_registry.mutex);
        
        // Call shutdown function if available
        if (subsystem->shutdown_function) {
            subsystem->shutdown_function();
        }
        
        // Wait for thread exit if applicable
        if (subsystem->main_thread && *subsystem->main_thread != 0) {
            pthread_join(*subsystem->main_thread, NULL);
            *subsystem->main_thread = 0;
        }
        
        // Reacquire lock to update final state
        pthread_mutex_lock(&subsystem_registry.mutex);
        
        // Mark as inactive
        update_subsystem_state(subsystem_id, SUBSYSTEM_INACTIVE);
        success = true;
    }
    
    pthread_mutex_unlock(&subsystem_registry.mutex);
    return success;
}

/*
 * Stop all subsystems in dependency-aware order.
 * Returns the number of subsystems successfully stopped.
 */
size_t stop_all_subsystems_in_dependency_order(void) {
    size_t stopped_count = 0;
    bool any_stopped;
    
    // Keep trying until no more subsystems can be stopped
    do {
        any_stopped = false;
        
        pthread_mutex_lock(&subsystem_registry.mutex);
        
        // Allocate array to track leaf subsystems based on current count
        bool* is_leaf = calloc(subsystem_registry.count, sizeof(bool));
        int leaf_count = 0;
        
        // Handle allocation failure
        if (!is_leaf) {
            log_this("Shutdown", "Failed to allocate memory for leaf subsystem tracking", LOG_LEVEL_ERROR);
            pthread_mutex_unlock(&subsystem_registry.mutex);
            return stopped_count;
        }
        
        for (int i = 0; i < subsystem_registry.count; i++) {
            SubsystemInfo* subsystem = &subsystem_registry.subsystems[i];
            if (subsystem->state == SUBSYSTEM_RUNNING) {
                bool has_dependents = false;
                
                // Check if any other running subsystem depends on this one
                for (int j = 0; j < subsystem_registry.count; j++) {
                    if (i != j && subsystem_registry.subsystems[j].state == SUBSYSTEM_RUNNING) {
                        for (int k = 0; k < subsystem_registry.subsystems[j].dependency_count; k++) {
                            if (strcmp(subsystem_registry.subsystems[j].dependencies[k], subsystem->name) == 0) {
                                has_dependents = true;
                                break;
                            }
                        }
                        if (has_dependents) break;
                    }
                }
                
                if (!has_dependents) {
                    is_leaf[i] = true;
                    leaf_count++;
                }
            }
        }
        
        pthread_mutex_unlock(&subsystem_registry.mutex);
        
        // Now stop all leaf subsystems
        if (leaf_count > 0) {
            for (int i = 0; i < subsystem_registry.count; i++) {
                if (is_leaf[i]) {
                    if (stop_subsystem_and_dependents(i)) {
                        stopped_count++;
                        any_stopped = true;
                    }
                }
            }
        }
        
        // Free the dynamically allocated array
        free(is_leaf);
        
        // Brief delay between iterations
        if (any_stopped) {
            usleep(100000);  // 100ms
        }
        
    } while (any_stopped);
    
    return stopped_count;
}

/*
 * Update the subsystem registry during shutdown.
 * This function marks all subsystems as stopping or inactive.
 */
void update_subsystem_registry_on_shutdown(void) {
    // Mark all running subsystems as stopping or inactive based on their thread count
    
    // Check Print Queue
    update_service_thread_metrics(&print_threads);
    if (print_threads.thread_count > 0) {
        update_subsystem_on_shutdown("PrintQueue");
    } else {
        update_subsystem_after_shutdown("PrintQueue");
    }
    
    // Check Terminal
    update_subsystem_after_shutdown("Terminal");
    
    // Check Swagger
    update_subsystem_after_shutdown("Swagger");
    
    // Check SMTP Relay
    update_subsystem_after_shutdown("SMTPRelay");
    
    // Check mDNS Client
    update_subsystem_after_shutdown("MDNSClient");
    
    // Check mDNS Server
    update_service_thread_metrics(&mdns_server_threads);
    if (mdns_server_threads.thread_count > 0) {
        update_subsystem_on_shutdown("MDNSServer");
    } else {
        update_subsystem_after_shutdown("MDNSServer");
    }
    
    // Check WebSocket
    update_service_thread_metrics(&websocket_threads);
    if (websocket_threads.thread_count > 0) {
        update_subsystem_on_shutdown("WebSocket");
    } else {
        update_subsystem_after_shutdown("WebSocket");
    }
    
    // Check Web Server
    update_service_thread_metrics(&web_threads);
    if (web_threads.thread_count > 0) {
        update_subsystem_on_shutdown("WebServer");
    } else {
        update_subsystem_after_shutdown("WebServer");
    }
    
    // Check Logging - Always last
    update_service_thread_metrics(&logging_threads);
    if (logging_threads.thread_count > 0) {
        update_subsystem_on_shutdown("Logging");
    } else {
        update_subsystem_after_shutdown("Logging");
    }
}

/*
 * Get a formatted string containing the status of all running subsystems.
 * Caller is responsible for freeing the returned buffer.
 * 
 * @param status_buffer Pointer to a char* where the status buffer will be stored
 * @return true if successful, false otherwise
 */
bool get_running_subsystems_status(char** status_buffer) {
    if (!status_buffer) {
        return false;
    }
    
    // Allocate a reasonably sized buffer
    *status_buffer = malloc(4096);
    if (!*status_buffer) {
        return false;
    }
    
    char* buffer = *status_buffer;
    buffer[0] = '\0';
    
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    // Count running subsystems
    int running_count = 0;
    for (int i = 0; i < subsystem_registry.count; i++) {
        if (subsystem_registry.subsystems[i].state == SUBSYSTEM_RUNNING) {
            running_count++;
        }
    }
    
    // Create header
    sprintf(buffer, "RUNNING SUBSYSTEMS (%d/%d):\n", 
            running_count, subsystem_registry.count);
    
    // Add each running subsystem
    for (int i = 0; i < subsystem_registry.count; i++) {
        SubsystemInfo* subsystem = &subsystem_registry.subsystems[i];
        
        if (subsystem->state == SUBSYSTEM_RUNNING) {
            // Calculate time running
            time_t now = time(NULL);
            time_t running_time = now - subsystem->state_changed;
            int hours = running_time / 3600;
            int minutes = (running_time % 3600) / 60;
            int seconds = running_time % 60;
            
            // Get thread count if available
            int thread_count = 0;
            if (subsystem->threads) {
                thread_count = subsystem->threads->thread_count;
            }
            
            // Append to buffer
            char entry[256];
            snprintf(entry, sizeof(entry), "  %s - Running for %02d:%02d:%02d - Threads: %d\n", 
                    subsystem->name, hours, minutes, seconds, thread_count);
            
            strcat(buffer, entry);
        }
    }
    
    pthread_mutex_unlock(&subsystem_registry.mutex);
    
    return true;
}