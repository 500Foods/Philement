/*
 * Registry Implementation
 *
 * This module implements a centralized registry for tracking the state
 * of all server subsystems with thread-safe operations.
 * 
 * The registry starts with an empty array of subsystems and is populated
 * dynamically as subsystems are registered during launch readiness checks.
 */

// Global includes 
#include "../hydrogen.h"

// Local includes
#include "registry.h"

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
    "Ready",
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
static bool grow_registry(size_t new_capacity) {
    if (new_capacity <= (size_t)subsystem_registry.capacity) {
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
                  (new_capacity - (size_t)subsystem_registry.capacity) * sizeof(SubsystemInfo));
        }
    }
    
    if (!new_subsystems) {
        // Memory allocation failed
        return false;
    }
    
    // Update the registry
    subsystem_registry.subsystems = new_subsystems;
    subsystem_registry.capacity = (int)new_capacity;
    
    return true;
}

/*
 * Initialize the registry.
 */
void init_registry(void) {

    log_this(SR_REGISTRY, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_REGISTRY, "REGISTRY INITIALIZATION", LOG_LEVEL_DEBUG, 0);
    log_this(SR_REGISTRY, "― Reinitializing Registry Mutex", LOG_LEVEL_DEBUG, 0);

    // For robust test isolation, destroy and reinitialize the mutex
    // This ensures clean state between test runs
    pthread_mutex_destroy(&subsystem_registry.mutex);
    pthread_mutex_init(&subsystem_registry.mutex, NULL);

    // Lock the mutex to ensure thread safety during cleanup
    MutexResult lock_result = MUTEX_LOCK(&subsystem_registry.mutex, SR_REGISTRY);
    if (lock_result == MUTEX_SUCCESS) {
        // Free any existing allocation
        if (subsystem_registry.subsystems) {
            log_this(SR_REGISTRY, "― Cleaning Subsystems", LOG_LEVEL_DEBUG, 0);

            // Free any dynamically allocated strings in the subsystems
            for (int i = 0; i < subsystem_registry.count; i++) {
                // Free the dynamically allocated name (created with strdup)
                if (subsystem_registry.subsystems[i].name) {
                    free((char*)subsystem_registry.subsystems[i].name);
                    subsystem_registry.subsystems[i].name = NULL;
                }

                // Free the dynamically allocated dependency names
                for (int j = 0; j < subsystem_registry.subsystems[i].dependency_count; j++) {
                    if (subsystem_registry.subsystems[i].dependencies[j]) {
                        free((char*)subsystem_registry.subsystems[i].dependencies[j]);
                        subsystem_registry.subsystems[i].dependencies[j] = NULL;
                    }
                }
            }
            free(subsystem_registry.subsystems);
            subsystem_registry.subsystems = NULL;
        }

        // Reset all fields to ensure complete cleanup
        subsystem_registry.subsystems = NULL;
        subsystem_registry.count = 0;
        subsystem_registry.capacity = 0;

        mutex_unlock(&subsystem_registry.mutex);
    }

    log_this(SR_REGISTRY, "REGISTRY INITIALIZATION COMPLETE", LOG_LEVEL_DEBUG, 0);
}

/*
 * Register a new subsystem with the registry.
 */
int register_subsystem(const char* name, ServiceThreads* threads,
                     pthread_t* main_thread, volatile sig_atomic_t* shutdown_flag,
                     int (*init_function)(void), void (*shutdown_function)(void)) {
    if (!name) {
        log_this(SR_REGISTRY, "Cannot register subsystem with NULL name", LOG_LEVEL_ERROR, 0);
        return -1;
    }

    MutexResult lock_result = MUTEX_LOCK(&subsystem_registry.mutex, SR_REGISTRY);
    if (lock_result != MUTEX_SUCCESS) {
        return -1;
    }

    // Check if we need to grow the registry
    if (subsystem_registry.count >= subsystem_registry.capacity) {
        // Double the capacity or use initial capacity if empty
        size_t new_capacity = (subsystem_registry.capacity == 0) ?
                           INITIAL_REGISTRY_CAPACITY :
                           ((size_t)subsystem_registry.capacity * 2);

        if (!grow_registry(new_capacity)) {
            log_this(SR_REGISTRY, "Cannot register subsystem '%s': memory allocation failed", LOG_LEVEL_ERROR, 1, name);
            mutex_unlock(&subsystem_registry.mutex);
            return -1;
        }
    }

    // Check if a subsystem with this name already exists
    for (int i = 0; i < subsystem_registry.count; i++) {
        if (subsystem_registry.subsystems[i].name &&
            strcmp(subsystem_registry.subsystems[i].name, name) == 0) {
            log_this(SR_REGISTRY, "Subsystem '%s' already registered", LOG_LEVEL_ERROR, 1, name);
            mutex_unlock(&subsystem_registry.mutex);
            return -1;
        }
    }

    // Register the new subsystem
    int id = subsystem_registry.count;
    subsystem_registry.subsystems[id].name = strdup(name);  // Create a copy of the name
    if (!subsystem_registry.subsystems[id].name) {
        log_this(SR_REGISTRY, "Cannot register subsystem '%s': failed to allocate name", LOG_LEVEL_ERROR, 1, name);
        mutex_unlock(&subsystem_registry.mutex);
        return -1;
    }
    subsystem_registry.subsystems[id].state = SUBSYSTEM_INACTIVE;
    subsystem_registry.subsystems[id].state_changed = time(NULL);
    subsystem_registry.subsystems[id].threads = threads;
    subsystem_registry.subsystems[id].main_thread = main_thread;
    subsystem_registry.subsystems[id].shutdown_flag = shutdown_flag;
    subsystem_registry.subsystems[id].init_function = init_function;
    subsystem_registry.subsystems[id].shutdown_function = shutdown_function;
    subsystem_registry.subsystems[id].dependency_count = 0;

    subsystem_registry.count++;

    mutex_unlock(&subsystem_registry.mutex);

    return id;
}

/*
 * Update the state of a subsystem with proper locking.
 */
void update_subsystem_state(int subsystem_id, SubsystemState new_state) {
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    if (subsystem_id >= 0 && subsystem_id < subsystem_registry.count) {
        SubsystemInfo* subsystem = &subsystem_registry.subsystems[subsystem_id];
        
        if (subsystem->state != new_state) {
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
        log_this(SR_REGISTRY, "Invalid subsystem ID: %d", LOG_LEVEL_ERROR, 1, subsystem_id);
        pthread_mutex_unlock(&subsystem_registry.mutex);
        return false;
    }
    
    SubsystemInfo* subsystem = &subsystem_registry.subsystems[subsystem_id];
    
    // Check if already running or starting
    if (subsystem->state == SUBSYSTEM_RUNNING || subsystem->state == SUBSYSTEM_STARTING) {
        log_this(SR_REGISTRY, "Subsystem '%s' is already %s", LOG_LEVEL_DEBUG, 2, subsystem->name, state_strings[subsystem->state]);
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
        log_this(SR_REGISTRY, "Cannot start subsystem '%s': missing dependencies: %s", LOG_LEVEL_ERROR, 2, subsystem->name, missing_deps);
        pthread_mutex_unlock(&subsystem_registry.mutex);
        return false;
    }
    
    // Mark as starting before releasing the lock
    subsystem->state = SUBSYSTEM_STARTING;
    subsystem->state_changed = time(NULL);
    
    pthread_mutex_unlock(&subsystem_registry.mutex);
    
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
    } else {
        update_subsystem_state(subsystem_id, SUBSYSTEM_ERROR);
        log_this(SR_REGISTRY, "Failed to start subsystem '%s'", LOG_LEVEL_ERROR, 1, subsystem->name);
    }
    
    return success;
}

/*
 * Stop a subsystem after checking for dependency violations.
 */
bool stop_subsystem(int subsystem_id) {
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    if (subsystem_id < 0 || subsystem_id >= subsystem_registry.count) {
        log_this(SR_REGISTRY, "Invalid subsystem ID: %d", LOG_LEVEL_ERROR, 1, subsystem_id);
        pthread_mutex_unlock(&subsystem_registry.mutex);
        return false;
    }
    
    SubsystemInfo* subsystem = &subsystem_registry.subsystems[subsystem_id];
    
    // Check if already stopped
    if (subsystem->state == SUBSYSTEM_INACTIVE) {
        log_this(SR_REGISTRY, "Subsystem '%s' is already inactive", LOG_LEVEL_DEBUG, 1, subsystem->name);
        pthread_mutex_unlock(&subsystem_registry.mutex);
        return true;
    }
    
    // Check for dependency violations - other subsystems that depend on this one
    char dependent_systems[256] = {0};
    int dependent_count = 0;
    
    for (int i = 0; i < subsystem_registry.count; i++) {
        if (i == subsystem_id) continue;

        const SubsystemInfo* other = &subsystem_registry.subsystems[i];
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
        log_this(SR_REGISTRY, "Cannot stop subsystem '%s': required by: %s", LOG_LEVEL_ERROR, 2, subsystem->name, dependent_systems);
        pthread_mutex_unlock(&subsystem_registry.mutex);
        return false;
    }
    
    // Mark as stopping before releasing the lock
    subsystem->state = SUBSYSTEM_STOPPING;
    subsystem->state_changed = time(NULL);
    
    pthread_mutex_unlock(&subsystem_registry.mutex);
    
    log_this(SR_REGISTRY, "Stopping subsystem '%s'", LOG_LEVEL_DEBUG, 1, subsystem->name);
    
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
    
    log_this(SR_REGISTRY, "Subsystem '%s' stopped successfully", LOG_LEVEL_DEBUG, 1, subsystem->name);
    
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
#ifdef UNITY_TEST_MODE
__attribute__((weak))
#endif
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
    if (!dependency_name || strlen(dependency_name) == 0) return false;
    
    pthread_mutex_lock(&subsystem_registry.mutex);
    
    bool success = false;
    if (subsystem_id >= 0 && subsystem_id < subsystem_registry.count) {
        SubsystemInfo* subsystem = &subsystem_registry.subsystems[subsystem_id];
        
        // Check if we've reached the maximum dependencies
        if (subsystem->dependency_count >= MAX_DEPENDENCIES) {
            log_this(SR_REGISTRY, "Cannot add dependency for '%s': maximum dependencies reached", LOG_LEVEL_ERROR, 1, subsystem->name);
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
                log_this(SR_REGISTRY, "Dependency '%s' already registered for '%s'", LOG_LEVEL_DEBUG, 1, dependency_name, subsystem->name);
                success = true;  // Core function returns true for duplicates (but doesn't add)
            } else {
                // Add the dependency (make a copy of the name)
                subsystem->dependencies[subsystem->dependency_count] = strdup(dependency_name);
                if (!subsystem->dependencies[subsystem->dependency_count]) {
                    log_this(SR_REGISTRY, "Failed to allocate memory for dependency name '%s'", LOG_LEVEL_ERROR, 1, dependency_name);
                    success = false;
                } else {
                    subsystem->dependency_count++;
                    success = true;
                }
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

    // Check if subsystem_id is valid first
    if (subsystem_id < 0 || subsystem_id >= subsystem_registry.count) {
        pthread_mutex_unlock(&subsystem_registry.mutex);
        return false; // Invalid subsystem ID
    }

    bool all_running = true;
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
    log_this(SR_REGISTRY, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_REGISTRY, "SUBSYSTEM STATUS REPORT", LOG_LEVEL_DEBUG, 0);
    log_this(SR_REGISTRY, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    
    time_t now = time(NULL);
    char time_buffer[64];
    int running_count = 0;
    
    for (int i = 0; i < subsystem_registry.count; i++) {
        SubsystemInfo* subsystem = &subsystem_registry.subsystems[i];
        
        // Calculate time in current state
        time_t time_in_state = now - subsystem->state_changed;
        int hours = (int)(time_in_state / 3600);
        int minutes = (int)((time_in_state % 3600) / 60);
        int seconds = (int)(time_in_state % 60);
        
        // Format the time string
        snprintf(time_buffer, sizeof(time_buffer), "%02d:%02d:%02d", hours, minutes, seconds);
        
        // Determine log level based on state
        int log_level = LOG_LEVEL_DEBUG;
        if (subsystem->state == SUBSYSTEM_ERROR) {
            log_level = LOG_LEVEL_ERROR;
        } else if (subsystem->state == SUBSYSTEM_STOPPING) {
            log_level = LOG_LEVEL_ALERT;
        }
        
        // Log the basic subsystem status
        log_this(SR_REGISTRY, "Subsystem: %s - State: %s - Time: %s", log_level, 3,
            subsystem->name, 
            state_strings[subsystem->state], 
            time_buffer);
        
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
            log_this(SR_REGISTRY, "  Dependencies: %s", LOG_LEVEL_DEBUG, 1, deps);
        }
        
        // Log thread information if available
        if (subsystem->threads) {
            update_service_thread_metrics(subsystem->threads);
            log_this(SR_REGISTRY, "  Threads: %d - Memory: %zu bytes", LOG_LEVEL_DEBUG, 2,
                subsystem->threads->thread_count, 
                subsystem->threads->resident_memory);
        }
    }
    
    // Summary information
    log_this(SR_REGISTRY, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_REGISTRY, "Total subsystems: %d - Running: %d", LOG_LEVEL_DEBUG, 2, subsystem_registry.count, running_count);
    log_this(SR_REGISTRY, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    
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

/*
 * Check if the registry is ready for launch.
 */
LaunchReadiness check_registry_readiness(void) {
    LaunchReadiness readiness = {0};
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // First message is always the subsystem name
    readiness.messages[0] = strdup(SR_REGISTRY);
    
    // Check if mutex is initialized (non-zero value)
    pthread_mutex_lock(&subsystem_registry.mutex);
    bool mutex_ok = (memcmp(&subsystem_registry.mutex, &(pthread_mutex_t){0}, sizeof(pthread_mutex_t)) != 0);
    pthread_mutex_unlock(&subsystem_registry.mutex);
    
    // Registry is ready if mutex is initialized
    // (subsystems array can be NULL initially - that's normal)
    readiness.ready = mutex_ok;
    
    // Add status message
    if (readiness.ready) {
        readiness.messages[1] = strdup("  Go:      " SR_REGISTRY " initialized");
    } else {
        readiness.messages[1] = strdup("  No-Go:   " SR_REGISTRY " not initialized");
    }
    
    // Add decision message
    readiness.messages[2] = strdup("  Decide:  Go For Launch of " SR_REGISTRY);
    readiness.messages[3] = NULL;  // NULL terminator
    
    return readiness;
}

/*
 * Get the number of dependencies for a subsystem.
 */
int get_subsystem_dependency_count(int subsystem_id) {
    pthread_mutex_lock(&subsystem_registry.mutex);

    int count = -1;
    if (subsystem_id >= 0 && subsystem_id < subsystem_registry.count) {
        count = subsystem_registry.subsystems[subsystem_id].dependency_count;
    }

    pthread_mutex_unlock(&subsystem_registry.mutex);

    return count;
}

/*
 * Get a dependency name by index for a subsystem.
 */
const char* get_subsystem_dependency(int subsystem_id, int dependency_index) {
    pthread_mutex_lock(&subsystem_registry.mutex);

    const char* dependency_name = NULL;
    if (subsystem_id >= 0 && subsystem_id < subsystem_registry.count) {
        SubsystemInfo* subsystem = &subsystem_registry.subsystems[subsystem_id];
        if (dependency_index >= 0 && dependency_index < subsystem->dependency_count) {
            dependency_name = subsystem->dependencies[dependency_index];
        }
    }

    pthread_mutex_unlock(&subsystem_registry.mutex);

    return dependency_name;
}
