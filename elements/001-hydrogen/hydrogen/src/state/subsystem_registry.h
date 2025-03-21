/*
 * Subsystem Registry for Hydrogen Server
 *
 * This module provides a centralized registry for tracking the state
 * of all server subsystems. It enables:
 *
 * 1. Runtime tracking of which subsystems are active
 * 2. Dependency management between subsystems
 * 3. Dynamic starting/stopping of subsystems after initial startup
 * 4. Comprehensive status reporting during shutdown
 */

#ifndef SUBSYSTEM_REGISTRY_H
#define SUBSYSTEM_REGISTRY_H

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// System includes
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>

// Project includes
#include "../utils/utils_threads.h"

// Constants
// Maximum dependencies per subsystem (could be made dynamic in the future)
#define MAX_DEPENDENCIES 8

// Initial capacity for the subsystem registry
#define INITIAL_REGISTRY_CAPACITY 8

// Subsystem state enum
typedef enum {
    SUBSYSTEM_INACTIVE,  // Not started
    SUBSYSTEM_STARTING,  // In the process of starting
    SUBSYSTEM_RUNNING,   // Running normally
    SUBSYSTEM_STOPPING,  // In the process of stopping
    SUBSYSTEM_ERROR      // Error state
} SubsystemState;

// Subsystem metadata structure
typedef struct {
    const char* name;              // Subsystem name
    SubsystemState state;          // Current state
    time_t state_changed;          // When the state last changed
    ServiceThreads* threads;       // Pointer to thread tracking structure
    pthread_t* main_thread;        // Pointer to main thread handle
    volatile sig_atomic_t* shutdown_flag; // Pointer to shutdown flag
    
    // Dependencies
    int dependency_count;
    const char* dependencies[MAX_DEPENDENCIES];
    
    // Callbacks
    int (*init_function)(void);    // Init function pointer (returns 1 on success, 0 on failure)
    void (*shutdown_function)(void); // Shutdown function pointer
} SubsystemInfo;

// Registry structure
typedef struct {
    SubsystemInfo* subsystems;     // Dynamically allocated array of subsystems
    int count;                     // Number of registered subsystems
    int capacity;                  // Current capacity of the subsystems array
    pthread_mutex_t mutex;         // For thread-safe access
} SubsystemRegistry;

// Global instance
extern SubsystemRegistry subsystem_registry;

/*
 * Initialize the subsystem registry.
 * Must be called before any other registry functions.
 */
void init_subsystem_registry(void);

/*
 * Register a new subsystem with the registry.
 *
 * @param name Subsystem name
 * @param threads Pointer to thread tracking structure
 * @param main_thread Pointer to main thread handle
 * @param shutdown_flag Pointer to shutdown flag
 * @param init_function Function to initialize the subsystem (returns int, 1 on success, 0 on failure)
 * @param shutdown_function Function to shutdown the subsystem
 * @return Subsystem ID or -1 on error
 */
int register_subsystem(const char* name, ServiceThreads* threads, 
                     pthread_t* main_thread, volatile sig_atomic_t* shutdown_flag,
                     int (*init_function)(void), void (*shutdown_function)(void));

/*
 * Start a subsystem.
 * Checks dependencies and calls the init function.
 *
 * @param subsystem_id ID of the subsystem to start
 * @return true if started successfully, false otherwise
 */
bool start_subsystem(int subsystem_id);

/*
 * Stop a subsystem.
 * Checks dependency violations and calls the shutdown function.
 *
 * @param subsystem_id ID of the subsystem to stop
 * @return true if stopped successfully, false otherwise
 */
bool stop_subsystem(int subsystem_id);

/*
 * Update the state of a subsystem.
 *
 * @param subsystem_id ID of the subsystem
 * @param new_state New state to set
 */
void update_subsystem_state(int subsystem_id, SubsystemState new_state);

/*
 * Check if a subsystem is running.
 *
 * @param subsystem_id ID of the subsystem
 * @return true if running, false otherwise
 */
bool is_subsystem_running(int subsystem_id);

/*
 * Check if a subsystem is running by name.
 *
 * @param name Name of the subsystem
 * @return true if running, false otherwise
 */
bool is_subsystem_running_by_name(const char* name);

/*
 * Get the current state of a subsystem.
 *
 * @param subsystem_id ID of the subsystem
 * @return The current state
 */
SubsystemState get_subsystem_state(int subsystem_id);

/*
 * Update the registry after a subsystem has been shut down.
 * This should be called after a subsystem's shutdown function has completed.
 *
 * @param subsystem_name Name of the subsystem that was shut down
 */
void update_subsystem_after_shutdown(const char* subsystem_name);

/*
 * Add a dependency to a subsystem.
 *
 * @param subsystem_id ID of the subsystem
 * @param dependency_name Name of the dependency
 * @return true if added successfully, false otherwise
 */
bool add_subsystem_dependency(int subsystem_id, const char* dependency_name);

/*
 * Check if all dependencies of a subsystem are running.
 *
 * @param subsystem_id ID of the subsystem
 * @return true if all dependencies are running, false otherwise
 */
bool check_subsystem_dependencies(int subsystem_id);

/*
 * Get the subsystem ID by name.
 *
 * @param name Name of the subsystem
 * @return Subsystem ID or -1 if not found
 */
int get_subsystem_id_by_name(const char* name);

/*
 * Print the status of all registered subsystems.
 * Used for diagnostic and monitoring purposes.
 */
void print_subsystem_status(void);

/*
 * Get a human-readable string for a subsystem state.
 *
 * @param state The subsystem state
 * @return String representation of the state
 */
const char* subsystem_state_to_string(SubsystemState state);

#endif /* SUBSYSTEM_REGISTRY_H */