/*
 * Subsystem Registry Integration Header
 *
 * This header defines the functions that integrate the subsystem registry
 * with the Hydrogen server's startup and shutdown processes.
 */

#ifndef SUBSYSTEM_REGISTRY_INTEGRATION_H
#define SUBSYSTEM_REGISTRY_INTEGRATION_H

// System includes
#include <stdbool.h>

// Project includes
#include "subsystem_registry.h"

/*
 * Initialize the registry subsystem.
 * This initializes the registry itself as the first subsystem.
 */
void initialize_registry_subsystem(void);

/*
 * Register a single subsystem based on its launch readiness result.
 * This is called during the Launch Go/No-Go process for subsystems that pass.
 * Returns the subsystem ID or -1 if registration failed.
 *
 * @param name Subsystem name
 * @param threads Pointer to thread tracking structure
 * @param main_thread Pointer to main thread handle
 * @param shutdown_flag Pointer to shutdown flag
 * @param init_function Function to initialize the subsystem (returns int, 1 on success, 0 on failure)
 * @param shutdown_function Function to shutdown the subsystem
 * @return Subsystem ID or -1 on error
 */
int register_subsystem_from_launch(const char* name, ServiceThreads* threads, 
                                  pthread_t* main_thread, volatile sig_atomic_t* shutdown_flag,
                                  int (*init_function)(void), void (*shutdown_function)(void));

/*
 * Add a dependency for a subsystem from the launch process.
 * This is called during the Launch Go/No-Go process for each dependency identified.
 *
 * @param subsystem_id ID of the subsystem
 * @param dependency_name Name of the dependency
 * @return true if added successfully, false otherwise
 */
bool add_dependency_from_launch(int subsystem_id, const char* dependency_name);

/* 
 * Legacy function - will be deprecated in future versions in favor of individual 
 * registrations via register_subsystem_from_launch.
 */
void register_standard_subsystems(void);

/*
 * Update the registry when a subsystem is started during the startup sequence.
 * This should be called after a subsystem's init function has been called.
 *
 * @param subsystem_name Name of the subsystem
 * @param success Whether the subsystem started successfully
 */
void update_subsystem_on_startup(const char* subsystem_name, bool success);

/*
 * Update the registry with all subsystems that were started during startup.
 * This synchronizes the registry with the actual state of the system.
 */
void update_subsystem_registry_on_startup(void);

/*
 * Update the registry when a subsystem is stopping during shutdown.
 * This should be called before a subsystem's shutdown function is called.
 *
 * @param subsystem_name Name of the subsystem
 */
void update_subsystem_on_shutdown(const char* subsystem_name);

/*
 * Update the registry after a subsystem has stopped during shutdown.
 * This should be called after a subsystem's shutdown function has returned.
 *
 * @param subsystem_name Name of the subsystem
 */
void update_subsystem_after_shutdown(const char* subsystem_name);

/*
 * Update the subsystem registry during shutdown.
 * This function marks all subsystems as stopping or inactive.
 */
void update_subsystem_registry_on_shutdown(void);

/*
 * Get a formatted string containing the status of all running subsystems.
 * Caller is responsible for freeing the returned buffer.
 * 
 * @param status_buffer Pointer to a char* where the status buffer will be stored
 * @return true if successful, false otherwise
 */
bool get_running_subsystems_status(char** status_buffer);

#endif /* SUBSYSTEM_REGISTRY_INTEGRATION_H */