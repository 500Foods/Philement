/*
 * Registry for Hydrogen Server
 *
 * This module provides a centralized registry for tracking the state
 * of all server subsystems. It enables:
 *
 * 1. Runtime tracking of which subsystems are active
 * 2. Dependency management between subsystems
 * 3. Dynamic starting/stopping of subsystems after initial startup
 * 4. Comprehensive status reporting during shutdown
 * 5. Dynamic registration of subsystems with names provided through the registration system
 */

#ifndef REGISTRY_H
#define REGISTRY_H

// System includes
#include "../globals.h"
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>

// Project includes
#include <src/threads/threads.h>
#include <src/state/state_types.h>  // For shared types
#include "registry_integration.h"  // For subsystem registration and integration

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
// Forward declarations for internal functions
bool grow_registry(size_t new_capacity);

// Global instance
extern SubsystemRegistry subsystem_registry;

/**
 * @brief Initialize the registry
 * @note AI-guidance: Thread-safe, must be called before any other registry functions
 */
void init_registry(void);

/**
 * @brief Register a new subsystem with the registry
 * @param name Subsystem name
 * @param threads Pointer to thread tracking structure
 * @param main_thread Pointer to main thread handle
 * @param shutdown_flag Pointer to shutdown flag
 * @param init_function Function to initialize the subsystem (returns int, 1 on success, 0 on failure)
 * @param shutdown_function Function to shutdown the subsystem
 * @return Subsystem ID or -1 on error
 * @note AI-guidance: Thread-safe, validates input parameters
 */
int register_subsystem(const char* name, ServiceThreads* threads, 
                     pthread_t* main_thread, volatile sig_atomic_t* shutdown_flag,
                     int (*init_function)(void), void (*shutdown_function)(void));

/**
 * @brief Start a subsystem
 * @param subsystem_id ID of the subsystem to start
 * @return true if started successfully, false otherwise
 * @note AI-guidance: Thread-safe, checks dependencies
 */
bool start_subsystem(int subsystem_id);

/**
 * @brief Stop a subsystem
 * @param subsystem_id ID of the subsystem to stop
 * @return true if stopped successfully, false otherwise
 * @note AI-guidance: Thread-safe, checks dependency violations
 */
bool stop_subsystem(int subsystem_id);

/**
 * @brief Update the state of a subsystem
 * @param subsystem_id ID of the subsystem
 * @param new_state New state to set
 * @note AI-guidance: Thread-safe
 */
void update_subsystem_state(int subsystem_id, SubsystemState new_state);

/**
 * @brief Check if a subsystem is running
 * @param subsystem_id ID of the subsystem
 * @return true if running, false otherwise
 * @note AI-guidance: Thread-safe
 */
bool is_subsystem_running(int subsystem_id);

/**
 * @brief Check if a subsystem is running by name
 * @param name Name of the subsystem
 * @return true if running, false otherwise
 * @note AI-guidance: Thread-safe
 */
bool is_subsystem_running_by_name(const char* name);

/**
 * @brief Get the current state of a subsystem
 * @param subsystem_id ID of the subsystem
 * @return The current state
 * @note AI-guidance: Thread-safe
 */
SubsystemState get_subsystem_state(int subsystem_id);

/**
 * @brief Update the registry after a subsystem has been shut down
 * @param subsystem_name Name of the subsystem that was shut down
 * @note AI-guidance: Thread-safe, call after shutdown completion
 */
void update_subsystem_after_shutdown(const char* subsystem_name);

/**
 * @brief Add a dependency to a subsystem
 * @param subsystem_id ID of the subsystem
 * @param dependency_name Name of the dependency
 * @return true if added successfully, false otherwise
 * @note AI-guidance: Thread-safe
 */
bool add_subsystem_dependency(int subsystem_id, const char* dependency_name);

/**
 * @brief Check if all dependencies of a subsystem are running
 * @param subsystem_id ID of the subsystem
 * @return true if all dependencies are running, false otherwise
 * @note AI-guidance: Thread-safe
 */
bool check_subsystem_dependencies(int subsystem_id);

/**
 * @brief Get the subsystem ID by name
 * @param name Name of the subsystem
 * @return Subsystem ID or -1 if not found
 * @note AI-guidance: Thread-safe
 */
int get_subsystem_id_by_name(const char* name);

/**
 * @brief Print the status of all registered subsystems
 * @note AI-guidance: Thread-safe, used for diagnostics
 */
void print_subsystem_status(void);

/**
 * @brief Get a human-readable string for a subsystem state
 * @param state The subsystem state
 * @return String representation of the state
 * @note AI-guidance: Thread-safe, const return value
 */
const char* subsystem_state_to_string(SubsystemState state);

/**
 * @brief Check if the registry is ready for launch
 * @return LaunchReadiness struct containing readiness state and messages
 * @note AI-guidance: Thread-safe, validates registry state
 */
LaunchReadiness check_registry_readiness(void);

/**
 * @brief Get the number of dependencies for a subsystem
 * @param subsystem_id ID of the subsystem
 * @return Number of dependencies or -1 if invalid ID
 * @note AI-guidance: Thread-safe, useful for testing and debugging
 */
int get_subsystem_dependency_count(int subsystem_id);

/**
 * @brief Get a dependency name by index for a subsystem
 * @param subsystem_id ID of the subsystem
 * @param dependency_index Index of the dependency (0-based)
 * @return Dependency name or NULL if invalid ID or index
 * @note AI-guidance: Thread-safe, useful for testing and debugging
 */
const char* get_subsystem_dependency(int subsystem_id, int dependency_index);

#endif // REGISTRY_H
