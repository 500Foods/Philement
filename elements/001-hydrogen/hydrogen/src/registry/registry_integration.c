/*
 * Registry Integration
 *
 * This module integrates the registry with the Hydrogen server's
 * startup and shutdown processes. It registers all standard subsystems
 * and manages their dependencies.
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "registry_integration.h"

/*
 * Initialize the registry.
 */
void initialize_registry(void) {
    init_registry();
}

/*
 * Register a single subsystem based on its launch readiness result.
 */
int register_subsystem_from_launch(const char* name, ServiceThreads* threads, 
                                pthread_t* main_thread, volatile sig_atomic_t* shutdown_flag,
                                int (*init_function)(void), void (*shutdown_function)(void)) {
    // Validate name is not NULL or empty
    if (!name || strlen(name) == 0) {
        log_this(SR_LAUNCH, "Cannot register subsystem with NULL or empty name", LOG_LEVEL_ERROR, 0);
        return -1;
    }
    
    int subsys_id = register_subsystem(name, threads, main_thread, shutdown_flag, 
                                    init_function, shutdown_function);
    
    if (subsys_id < 0) {
        log_this(SR_LAUNCH, "Failed to register subsystem '%s'", LOG_LEVEL_ERROR, 1, name);
    }
    
    return subsys_id;
}

/*
 * Add a dependency for a subsystem from the launch process.
 */
bool add_dependency_from_launch(int subsystem_id, const char* dependency_name) {
    // Validate dependency name is not NULL or empty
    if (!dependency_name || strlen(dependency_name) == 0) {
        log_this(SR_LAUNCH, "Cannot add NULL or empty dependency name", LOG_LEVEL_ERROR, 0);
        return false;
    }
    
    bool result = add_subsystem_dependency(subsystem_id, dependency_name);
    
    if (!result) {
        log_this(SR_LAUNCH, "Failed to add dependency '%s' to subsystem", LOG_LEVEL_ERROR, 1, dependency_name);
    }
    
    return result;
}

/*
 * Update the registry when a subsystem is started.
 */
void update_subsystem_on_startup(const char* subsystem_name, bool success) {
    int id = get_subsystem_id_by_name(subsystem_name);
    if (id >= 0) {
        update_subsystem_state(id, success ? SUBSYSTEM_RUNNING : SUBSYSTEM_ERROR);
    }
}

/*
 * Update the registry with all started subsystems.
 */
void update_registry_on_startup(void) {
    // Check each subsystem for evidence it's running
    
    // Logging - Always starts first
    update_service_thread_metrics(&logging_threads);
    update_subsystem_on_startup(SR_LOGGING, logging_threads.thread_count > 0);
    
    // Web Server
    update_service_thread_metrics(&webserver_threads);
    update_subsystem_on_startup(SR_WEBSERVER, webserver_threads.thread_count > 0);
    
    // WebSocket
    update_service_thread_metrics(&websocket_threads);
    update_subsystem_on_startup(SR_WEBSOCKET, websocket_threads.thread_count > 0);
    
    // mDNS Server
    update_service_thread_metrics(&mdns_server_threads);
    update_subsystem_on_startup(SR_MDNS_SERVER, mdns_server_threads.thread_count > 0);
    
    // mDNS Client - No thread, check if not shutdown
    update_subsystem_on_startup(SR_MDNS_CLIENT, app_config && !mdns_client_system_shutdown);
    
    // Mail Relay - No thread, check if not shutdown
    update_subsystem_on_startup(SR_MAIL_RELAY, app_config && !mail_relay_system_shutdown);
    
    // Swagger - No thread, check if not shutdown
    update_subsystem_on_startup(SR_SWAGGER, app_config && !swagger_system_shutdown);
    
    // Terminal - No thread, check if not shutdown
    update_subsystem_on_startup(SR_TERMINAL, app_config && !terminal_system_shutdown);
    
    // Print Queue
    update_service_thread_metrics(&print_threads);
    update_subsystem_on_startup(SR_PRINT, print_threads.thread_count > 0);

}

/*
 * Update the registry when a subsystem is stopping.
 */
void update_subsystem_on_shutdown(const char* subsystem_name) {
    int id = get_subsystem_id_by_name(subsystem_name);
    if (id >= 0) {
        update_subsystem_state(id, SUBSYSTEM_STOPPING);
    }
}

/*
 * Update the registry after a subsystem has stopped.
 */
void update_subsystem_after_shutdown(const char* subsystem_name) {
    int id = get_subsystem_id_by_name(subsystem_name);
    if (id >= 0) {
        update_subsystem_state(id, SUBSYSTEM_INACTIVE);
    }
}

/*
 * Stop a subsystem and all its dependents safely.
 */
bool stop_subsystem_and_dependents(int subsystem_id) {
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    // Check if subsystem_id is valid
    if (subsystem_id < 0 || subsystem_id >= subsystem_registry.count) {
        pthread_mutex_unlock(&subsystem_registry.mutex);
        return false;
    }
    
    SubsystemInfo* subsystem = &subsystem_registry.subsystems[subsystem_id];
    bool success = true;
    
    // First check if any other running subsystems depend on this one
    for (int i = 0; i < subsystem_registry.count; i++) {
        const SubsystemInfo* other = &subsystem_registry.subsystems[i];
        if (other->state == SUBSYSTEM_RUNNING) {
            for (int j = 0; j < other->dependency_count; j++) {
                if (strcmp(other->dependencies[j], subsystem->name) == 0) {
                    // Found a dependent - stop it first
                    pthread_mutex_unlock(&subsystem_registry.mutex);
                    success &= stop_subsystem_and_dependents(i);
                    pthread_mutex_lock(&subsystem_registry.mutex);
                    // Re-check bounds after recursive call
                    if (subsystem_id >= subsystem_registry.count) {
                        pthread_mutex_unlock(&subsystem_registry.mutex);
                        return false;
                    }
                    // Refresh subsystem pointer after potential reallocation
                    subsystem = &subsystem_registry.subsystems[subsystem_id];
                }
            }
        }
    }
    
    // Now safe to stop this subsystem
    if (subsystem->state == SUBSYSTEM_RUNNING) {
        // Update state directly while holding mutex (avoid deadlock)
        subsystem->state = SUBSYSTEM_STOPPING;
        subsystem->state_changed = time(NULL);
        
        // Call shutdown function while holding mutex to ensure thread safety
        if (subsystem->shutdown_function) {
            pthread_mutex_unlock(&subsystem_registry.mutex);
            subsystem->shutdown_function();
            pthread_mutex_lock(&subsystem_registry.mutex);
            // Refresh subsystem pointer after potential reallocation
            subsystem = &subsystem_registry.subsystems[subsystem_id];
        }
        
        // Join main thread if provided
        if (subsystem->main_thread && *subsystem->main_thread != 0) {
            pthread_mutex_unlock(&subsystem_registry.mutex);
            pthread_join(*subsystem->main_thread, NULL);
            pthread_mutex_lock(&subsystem_registry.mutex);
            // Refresh subsystem pointer after potential reallocation
            subsystem = &subsystem_registry.subsystems[subsystem_id];
            *subsystem->main_thread = 0;
        }
        
        // Update state directly while holding mutex (avoid deadlock)
        subsystem->state = SUBSYSTEM_INACTIVE;
        subsystem->state_changed = time(NULL);
    } else if (subsystem->state == SUBSYSTEM_INACTIVE) {
        // Already stopped, but this is still considered success
        success = true;
    }
    
    pthread_mutex_unlock(&subsystem_registry.mutex);
    return success;
}

/*
 * Stop all subsystems in dependency-aware order.
 */
size_t stop_all_subsystems_in_dependency_order(void) {
    size_t stopped_count = 0;
    bool any_stopped;
    
    do {
        any_stopped = false;
        
        pthread_mutex_lock(&subsystem_registry.mutex);
        
        // Find all running subsystems that have no running dependents (leaf nodes)
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
                
                // If no dependents, this is a leaf node - stop it
                if (!has_dependents) {
                    // Store the shutdown function pointer before updating state
                    void (*shutdown_func)(void) = subsystem->shutdown_function;
                    pthread_t* main_thread = subsystem->main_thread;
                    
                    // Update state to stopping
                    subsystem->state = SUBSYSTEM_STOPPING;
                    subsystem->state_changed = time(NULL);
                    
                    pthread_mutex_unlock(&subsystem_registry.mutex);
                    
                    // Call shutdown function
                    if (shutdown_func) {
                        shutdown_func();
                    }
                    
                    // Join main thread if provided
                    if (main_thread && *main_thread != 0) {
                        pthread_join(*main_thread, NULL);
                        *main_thread = 0;
                    }
                    
                    // Update final state to inactive
                    pthread_mutex_lock(&subsystem_registry.mutex);
                    if (i < subsystem_registry.count) {
                        subsystem_registry.subsystems[i].state = SUBSYSTEM_INACTIVE;
                        subsystem_registry.subsystems[i].state_changed = time(NULL);
                    }
                    
                    stopped_count++;
                    any_stopped = true;
                    
                    // Break out of the loop since we changed the registry state
                    break;
                }
            }
        }
        
        pthread_mutex_unlock(&subsystem_registry.mutex);
        
        if (any_stopped) {
            usleep(10000);  // 10ms delay between rounds
        }
        
    } while (any_stopped);
    
    return stopped_count;
}

/*
 * Update the registry during shutdown.
 */
void update_registry_on_shutdown(void) {
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
    
    // Check Mail Relay
    update_subsystem_after_shutdown("MailRelay");
    
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
    update_service_thread_metrics(&webserver_threads);
    if (webserver_threads.thread_count > 0) {
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
 */
bool get_running_subsystems_status(char** status_buffer) {
    if (!status_buffer) {
        return false;
    }
    
    *status_buffer = malloc(4096);
    if (!*status_buffer) {
        return false;
    }
    
    char* buffer = *status_buffer;
    buffer[0] = '\0';
    
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    int running_count = 0;
    for (int i = 0; i < subsystem_registry.count; i++) {
        if (subsystem_registry.subsystems[i].state == SUBSYSTEM_RUNNING) {
            running_count++;
        }
    }
    
    sprintf(buffer, "RUNNING SUBSYSTEMS (%d/%d):\n", 
            running_count, subsystem_registry.count);
    
    for (int i = 0; i < subsystem_registry.count; i++) {
        SubsystemInfo* subsystem = &subsystem_registry.subsystems[i];
        
        if (subsystem->state == SUBSYSTEM_RUNNING) {
            time_t now = time(NULL);
            time_t running_time = now - subsystem->state_changed;
            int hours = (int)(running_time / 3600);
            int minutes = (int)((running_time % 3600) / 60);
            int seconds = (int)(running_time % 60);
            
            int thread_count = 0;
            if (subsystem->threads) {
                thread_count = subsystem->threads->thread_count;
            }
            
            char entry[256];
            snprintf(entry, sizeof(entry), "  %s - Running for %02d:%02d:%02d - Threads: %d\n", 
                    subsystem->name, hours, minutes, seconds, thread_count);
            
            strcat(buffer, entry);
        }
    }
    
    pthread_mutex_unlock(&subsystem_registry.mutex);
    
    return true;
}
