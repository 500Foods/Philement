/*
 * Subsystem Registry Implementation
 *
 * This module implements a centralized registry for tracking the state
 * of all server subsystems with thread-safe operations.
 * 
 * The registry starts with an empty array of subsystems and is populated
 * dynamically as subsystems are registered during launch readiness checks.
 */

// System includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

// Project includes
#include "subsystem_registry.h"
#include "../../logging/logging.h"
#include "../../utils/utils_threads.h"

// Global registry instance - initialized with empty array
SubsystemRegistry subsystem_registry = { 
    .subsystems = NULL,
    .count = 0,
    .capacity = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

// State string mapping for human-readable output
static const char* state_strings[] = {
    "Inactive",
    "Starting",
    "Running",
    "Stopping",
    "Error"
};

/*
 * Grow the registry capacity to accommodate more subsystems.
 * Must be called with the registry mutex locked.
 * 
 * @param new_capacity The new capacity to grow to
 * @return true if successful, false if allocation failed
 */
static bool grow_registry(int new_capacity) {
    if (new_capacity <= subsystem_registry.capacity) {
        return true;  // Already large enough
    }
    
    // Allocate a new, larger array
    SubsystemInfo* new_subsystems = NULL;
    
    if (subsystem_registry.subsystems == NULL) {
        // First allocation
        new_subsystems = calloc(new_capacity, sizeof(SubsystemInfo));
    } else {
        // Grow existing array
        new_subsystems = realloc(subsystem_registry.subsystems, 
                               new_capacity * sizeof(SubsystemInfo));
        
        // Initialize the new elements to zero
        if (new_subsystems) {
            memset(&new_subsystems[subsystem_registry.capacity], 0, 
                  (new_capacity - subsystem_registry.capacity) * sizeof(SubsystemInfo));
        }
    }
    
    if (!new_subsystems) {
        // Memory allocation failed
        return false;
    }
    
    // Update the registry
    subsystem_registry.subsystems = new_subsystems;
    subsystem_registry.capacity = new_capacity;
    
    return true;
}

/*
 * Initialize the subsystem registry.
 */
void init_subsystem_registry(void) {
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    // Free any existing allocation (just in case)
    if (subsystem_registry.subsystems) {
        free(subsystem_registry.subsystems);
        subsystem_registry.subsystems = NULL;
    }
    
    // Start with an empty array - don't allocate until needed
    subsystem_registry.subsystems = NULL;
    subsystem_registry.count = 0;
    subsystem_registry.capacity = 0;
    
    pthread_mutex_unlock(&subsystem_registry.mutex);
    
    // Registry initialization is now handled silently - no debug message needed
    // Output is managed by the launch system through the Go/No-Go process
}

/*
 * Register a new subsystem with the registry.
 */
int register_subsystem(const char* name, ServiceThreads* threads, 
                     pthread_t* main_thread, volatile sig_atomic_t* shutdown_flag,
                     int (*init_function)(void), void (*shutdown_function)(void)) {
    if (!name) {
        log_this("SubsysReg", "Cannot register subsystem with NULL name", LOG_LEVEL_ERROR);
        return -1;
    }
    
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    // Check if we need to grow the registry
    if (subsystem_registry.count >= subsystem_registry.capacity) {
        // Double the capacity or use initial capacity if empty
        int new_capacity = (subsystem_registry.capacity == 0) ? 
                           INITIAL_REGISTRY_CAPACITY : 
                           (subsystem_registry.capacity * 2);
        
        if (!grow_registry(new_capacity)) {
            log_this("SubsysReg", "Cannot register subsystem '%s': memory allocation failed", 
                    LOG_LEVEL_ERROR, name);
            pthread_mutex_unlock(&subsystem_registry.mutex);
            return -1;
        }
    }
    
    // Check if a subsystem with this name already exists
    for (int i = 0; i < subsystem_registry.count; i++) {
        if (subsystem_registry.subsystems[i].name && 
            strcmp(subsystem_registry.subsystems[i].name, name) == 0) {
            log_this("SubsysReg", "Subsystem '%s' already registered", LOG_LEVEL_ERROR, name);
            pthread_mutex_unlock(&subsystem_registry.mutex);
            return -1;
        }
    }
    
    // Register the new subsystem
    int id = subsystem_registry.count;
    subsystem_registry.subsystems[id].name = name;
    subsystem_registry.subsystems[id].state = SUBSYSTEM_INACTIVE;
    subsystem_registry.subsystems[id].state_changed = time(NULL);
    subsystem_registry.subsystems[id].threads = threads;
    subsystem_registry.subsystems[id].main_thread = main_thread;
    subsystem_registry.subsystems[id].shutdown_flag = shutdown_flag;
    subsystem_registry.subsystems[id].init_function = init_function;
    subsystem_registry.subsystems[id].shutdown_function = shutdown_function;
    subsystem_registry.subsystems[id].dependency_count = 0;
    
    subsystem_registry.count++;
    
    pthread_mutex_unlock(&subsystem_registry.mutex);
    
    // The "Decide" line in the launch readiness output already strongly implies the subsystem's status
    // log_this("SubsysReg", "Registered subsystem '%s' with ID %d", LOG_LEVEL_STATE, name, id);
    
    return id;
}
/*
 * Update the state of a subsystem with proper locking.
 */
void update_subsystem_state(int subsystem_id, SubsystemState new_state) {
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    if (subsystem_id >= 0 && subsystem_id < subsystem_registry.count) {
        SubsystemInfo* subsystem = &subsystem_registry.subsystems[subsystem_id];
        
        // Only log if the state is actually changing
        if (subsystem->state != new_state) {
            log_this("SubsysReg", "Subsystem '%s' changing state: %s -> %s", 
                    LOG_LEVEL_STATE, 
                    subsystem->name,
                    state_strings[subsystem->state], 
                    state_strings[new_state]);
            
            subsystem->state = new_state;
            subsystem->state_changed = time(NULL);
        }
    }
    
    pthread_mutex_unlock(&subsystem_registry.mutex);
}

/*
 * Start a subsystem after checking dependencies.
 */
bool start_subsystem(int subsystem_id) {
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    if (subsystem_id < 0 || subsystem_id >= subsystem_registry.count) {
        log_this("SubsysReg", "Invalid subsystem ID: %d", LOG_LEVEL_ERROR, subsystem_id);
        pthread_mutex_unlock(&subsystem_registry.mutex);
        return false;
    }
    
    SubsystemInfo* subsystem = &subsystem_registry.subsystems[subsystem_id];
    
    // Check if already running or starting
    if (subsystem->state == SUBSYSTEM_RUNNING || subsystem->state == SUBSYSTEM_STARTING) {
        log_this("SubsysReg", "Subsystem '%s' is already %s", 
                LOG_LEVEL_DEBUG, subsystem->name, state_strings[subsystem->state]);
        pthread_mutex_unlock(&subsystem_registry.mutex);
        return true;
    }
    
    // Check dependencies
    bool dependencies_met = true;
    char missing_deps[256] = {0};
    int missing_count = 0;
    
    for (int i = 0; i < subsystem->dependency_count; i++) {
        const char* dep_name = subsystem->dependencies[i];
        int dep_id = -1;
        
        // Find the dependency in the registry
        for (int j = 0; j < subsystem_registry.count; j++) {
            if (strcmp(subsystem_registry.subsystems[j].name, dep_name) == 0) {
                dep_id = j;
                break;
            }
        }
        
        // Check if the dependency is running
        if (dep_id == -1 || subsystem_registry.subsystems[dep_id].state != SUBSYSTEM_RUNNING) {
            dependencies_met = false;
            if (missing_count == 0) {
                strcat(missing_deps, dep_name);
            } else {
                strcat(missing_deps, ", ");
                strcat(missing_deps, dep_name);
            }
            missing_count++;
        }
    }
    
    if (!dependencies_met) {
        log_this("SubsysReg", "Cannot start subsystem '%s': missing dependencies: %s", 
                LOG_LEVEL_ERROR, subsystem->name, missing_deps);
        pthread_mutex_unlock(&subsystem_registry.mutex);
        return false;
    }
    
    // Mark as starting before releasing the lock
    subsystem->state = SUBSYSTEM_STARTING;
    subsystem->state_changed = time(NULL);
    
    pthread_mutex_unlock(&subsystem_registry.mutex);
    
    log_this("SubsysReg", "Starting subsystem '%s'", LOG_LEVEL_STATE, subsystem->name);
    
    // Call the initialization function
    bool success = false;
    if (subsystem->init_function) {
        // Convert from int return (1=success, 0=failure) to bool
        success = (subsystem->init_function() != 0);
    } else {
        // No init function is considered a success
        success = true;
    }
    
    // Update the state based on the result
    if (success) {
        update_subsystem_state(subsystem_id, SUBSYSTEM_RUNNING);
        log_this("SubsysReg", "Subsystem '%s' started successfully", LOG_LEVEL_STATE, subsystem->name);
    } else {
        update_subsystem_state(subsystem_id, SUBSYSTEM_ERROR);
        log_this("SubsysReg", "Failed to start subsystem '%s'", LOG_LEVEL_ERROR, subsystem->name);
    }
    
    return success;
}

/*
 * Stop a subsystem after checking for dependency violations.
 */
bool stop_subsystem(int subsystem_id) {
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    if (subsystem_id < 0 || subsystem_id >= subsystem_registry.count) {
        log_this("SubsysReg", "Invalid subsystem ID: %d", LOG_LEVEL_ERROR, subsystem_id);
        pthread_mutex_unlock(&subsystem_registry.mutex);
        return false;
    }
    
    SubsystemInfo* subsystem = &subsystem_registry.subsystems[subsystem_id];
    
    // Check if already stopped
    if (subsystem->state == SUBSYSTEM_INACTIVE) {
        log_this("SubsysReg", "Subsystem '%s' is already inactive", LOG_LEVEL_DEBUG, subsystem->name);
        pthread_mutex_unlock(&subsystem_registry.mutex);
        return true;
    }
    
    // Check for dependency violations - other subsystems that depend on this one
    char dependent_systems[256] = {0};
    int dependent_count = 0;
    
    for (int i = 0; i < subsystem_registry.count; i++) {
        if (i == subsystem_id) continue;
        
        SubsystemInfo* other = &subsystem_registry.subsystems[i];
        if (other->state != SUBSYSTEM_RUNNING && other->state != SUBSYSTEM_STARTING) {
            continue;
        }
        
        for (int j = 0; j < other->dependency_count; j++) {
            if (strcmp(other->dependencies[j], subsystem->name) == 0) {
                if (dependent_count == 0) {
                    strcat(dependent_systems, other->name);
                } else {
                    strcat(dependent_systems, ", ");
                    strcat(dependent_systems, other->name);
                }
                dependent_count++;
                break;
            }
        }
    }
    
    if (dependent_count > 0) {
        log_this("SubsysReg", "Cannot stop subsystem '%s': required by: %s", 
                LOG_LEVEL_ERROR, subsystem->name, dependent_systems);
        pthread_mutex_unlock(&subsystem_registry.mutex);
        return false;
    }
    
    // Mark as stopping before releasing the lock
    subsystem->state = SUBSYSTEM_STOPPING;
    subsystem->state_changed = time(NULL);
    
    pthread_mutex_unlock(&subsystem_registry.mutex);
    
    log_this("SubsysReg", "Stopping subsystem '%s'", LOG_LEVEL_STATE, subsystem->name);
    
    // Set the shutdown flag if provided
    if (subsystem->shutdown_flag) {
        *subsystem->shutdown_flag = 1;
    }
    
    // Call the shutdown function if provided
    if (subsystem->shutdown_function) {
        subsystem->shutdown_function();
    }
    
    // Wait for the main thread to exit if provided
    if (subsystem->main_thread && *subsystem->main_thread != 0) {
        pthread_join(*subsystem->main_thread, NULL);
        *subsystem->main_thread = 0;
    }
    
    // Update state to inactive
    update_subsystem_state(subsystem_id, SUBSYSTEM_INACTIVE);
    
    log_this("SubsysReg", "Subsystem '%s' stopped successfully", LOG_LEVEL_STATE, subsystem->name);
    
    return true;
}

/*
 * Check if a subsystem is running.
 */
bool is_subsystem_running(int subsystem_id) {
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    bool running = false;
    if (subsystem_id >= 0 && subsystem_id < subsystem_registry.count) {
        running = (subsystem_registry.subsystems[subsystem_id].state == SUBSYSTEM_RUNNING);
    }
    
    pthread_mutex_unlock(&subsystem_registry.mutex);
    
    return running;
}

/*
 * Check if a subsystem is running by name.
 */
bool is_subsystem_running_by_name(const char* name) {
    if (!name) return false;
    
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    bool running = false;
    for (int i = 0; i < subsystem_registry.count; i++) {
        if (strcmp(subsystem_registry.subsystems[i].name, name) == 0) {
            running = (subsystem_registry.subsystems[i].state == SUBSYSTEM_RUNNING);
            break;
        }
    }
    
    pthread_mutex_unlock(&subsystem_registry.mutex);
    
    return running;
}

/*
 * Get the current state of a subsystem.
 */
SubsystemState get_subsystem_state(int subsystem_id) {
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    SubsystemState state = SUBSYSTEM_INACTIVE;
    if (subsystem_id >= 0 && subsystem_id < subsystem_registry.count) {
        state = subsystem_registry.subsystems[subsystem_id].state;
    }
    
    pthread_mutex_unlock(&subsystem_registry.mutex);
    
    return state;
}

/*
 * Add a dependency to a subsystem.
 */
bool add_subsystem_dependency(int subsystem_id, const char* dependency_name) {
    if (!dependency_name) return false;
    
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    bool success = false;
    if (subsystem_id >= 0 && subsystem_id < subsystem_registry.count) {
        SubsystemInfo* subsystem = &subsystem_registry.subsystems[subsystem_id];
        
        // Check if we've reached the maximum dependencies
        if (subsystem->dependency_count >= MAX_DEPENDENCIES) {
            log_this("SubsysReg", "Cannot add dependency for '%s': maximum dependencies reached", 
                    LOG_LEVEL_ERROR, subsystem->name);
        } 
        // Check if the dependency already exists
        else {
            bool already_exists = false;
            for (int i = 0; i < subsystem->dependency_count; i++) {
                if (strcmp(subsystem->dependencies[i], dependency_name) == 0) {
                    already_exists = true;
                    break;
                }
            }
            
            if (already_exists) {
                log_this("SubsysReg", "Dependency '%s' already registered for '%s'", 
                        LOG_LEVEL_DEBUG, dependency_name, subsystem->name);
                success = true;
            } else {
                // Add the dependency
                subsystem->dependencies[subsystem->dependency_count] = dependency_name;
                subsystem->dependency_count++;
                success = true;
                
                // log_this("SubsysReg", "Added dependency '%s' to subsystem '%s'", 
                //         LOG_LEVEL_STATE, dependency_name, subsystem->name);
            }
        }
    }
    
    pthread_mutex_unlock(&subsystem_registry.mutex);
    
    return success;
}

/*
 * Check if all dependencies of a subsystem are running.
 */
bool check_subsystem_dependencies(int subsystem_id) {
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    bool all_running = true;
    if (subsystem_id >= 0 && subsystem_id < subsystem_registry.count) {
        SubsystemInfo* subsystem = &subsystem_registry.subsystems[subsystem_id];
        
        for (int i = 0; i < subsystem->dependency_count; i++) {
            const char* dep_name = subsystem->dependencies[i];
            bool dep_running = false;
            
            for (int j = 0; j < subsystem_registry.count; j++) {
                if (strcmp(subsystem_registry.subsystems[j].name, dep_name) == 0) {
                    dep_running = (subsystem_registry.subsystems[j].state == SUBSYSTEM_RUNNING);
                    break;
                }
            }
            
            if (!dep_running) {
                all_running = false;
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&subsystem_registry.mutex);
    
    return all_running;
}

/*
 * Get the subsystem ID by name.
 */
int get_subsystem_id_by_name(const char* name) {
    if (!name) return -1;
    
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    int id = -1;
    for (int i = 0; i < subsystem_registry.count; i++) {
        if (strcmp(subsystem_registry.subsystems[i].name, name) == 0) {
            id = i;
            break;
        }
    }
    
    pthread_mutex_unlock(&subsystem_registry.mutex);
    
    return id;
}

/*
 * Print the status of all registered subsystems.
 */
void print_subsystem_status(void) {
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    // Header for the status report
    log_this("SubsysReg", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("SubsysReg", "SUBSYSTEM STATUS REPORT", LOG_LEVEL_STATE);
    log_this("SubsysReg", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    
    time_t now = time(NULL);
    char time_buffer[64];
    int running_count = 0;
    
    for (int i = 0; i < subsystem_registry.count; i++) {
        SubsystemInfo* subsystem = &subsystem_registry.subsystems[i];
        
        // Calculate time in current state
        time_t time_in_state = now - subsystem->state_changed;
        int hours = time_in_state / 3600;
        int minutes = (time_in_state % 3600) / 60;
        int seconds = time_in_state % 60;
        
        // Format the time string
        snprintf(time_buffer, sizeof(time_buffer), "%02d:%02d:%02d", hours, minutes, seconds);
        
        // Determine log level based on state
        int log_level = LOG_LEVEL_STATE;
        if (subsystem->state == SUBSYSTEM_ERROR) {
            log_level = LOG_LEVEL_ERROR;
        } else if (subsystem->state == SUBSYSTEM_STOPPING) {
            log_level = LOG_LEVEL_ALERT;
        }
        
        // Log the basic subsystem status
        log_this("SubsysReg", "Subsystem: %s - State: %s - Time: %s", 
                log_level, subsystem->name, state_strings[subsystem->state], time_buffer);
        
        // Count running subsystems
        if (subsystem->state == SUBSYSTEM_RUNNING) {
            running_count++;
        }
        
        // Log dependencies if any
        if (subsystem->dependency_count > 0) {
            char deps[256] = {0};
            for (int j = 0; j < subsystem->dependency_count; j++) {
                if (j > 0) strcat(deps, ", ");
                strcat(deps, subsystem->dependencies[j]);
            }
            log_this("SubsysReg", "  Dependencies: %s", LOG_LEVEL_STATE, deps);
        }
        
        // Log thread information if available
        if (subsystem->threads) {
            update_service_thread_metrics(subsystem->threads);
            log_this("SubsysReg", "  Threads: %d - Memory: %zu bytes", 
                    LOG_LEVEL_STATE, subsystem->threads->thread_count, 
                    subsystem->threads->resident_memory);
        }
    }
    
    // Summary information
    log_this("SubsysReg", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("SubsysReg", "Total subsystems: %d - Running: %d", 
            LOG_LEVEL_STATE, subsystem_registry.count, running_count);
    log_this("SubsysReg", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    
    pthread_mutex_unlock(&subsystem_registry.mutex);
}

/*
 * Get a human-readable string for a subsystem state.
 */
const char* subsystem_state_to_string(SubsystemState state) {
    if (state < SUBSYSTEM_INACTIVE || state > SUBSYSTEM_ERROR) {
        return "Unknown";
    }
    return state_strings[state];
}