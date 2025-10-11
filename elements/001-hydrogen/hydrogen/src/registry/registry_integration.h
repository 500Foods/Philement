/*
 * Registry Integration Header
 *
 * This header defines the functions that integrate the registry
 * with the Hydrogen server's startup and shutdown processes.
 */

#ifndef REGISTRY_INTEGRATION_H
#define REGISTRY_INTEGRATION_H

// System includes
#include <stdbool.h>

// Project includes
#include <src/config/config.h>
#include "registry.h"

/**
 * @brief Initialize the registry subsystem
 * @note AI-guidance: First subsystem to be initialized
 */
void initialize_registry(void);

/**
 * @brief Register a single subsystem based on its launch readiness result
 * @param name Subsystem name
 * @param threads Pointer to thread tracking structure
 * @param main_thread Pointer to main thread handle
 * @param shutdown_flag Pointer to shutdown flag
 * @param init_function Function to initialize the subsystem (returns int, 1 on success, 0 on failure)
 * @param shutdown_function Function to shutdown the subsystem
 * @return Subsystem ID or -1 on error
 * @note AI-guidance: Called during Launch Go/No-Go process
 */
int register_subsystem_from_launch(const char* name, ServiceThreads* threads, 
                                pthread_t* main_thread, volatile sig_atomic_t* shutdown_flag,
                                int (*init_function)(void), void (*shutdown_function)(void));

/**
 * @brief Add a dependency for a subsystem from the launch process
 * @param subsystem_id ID of the subsystem
 * @param dependency_name Name of the dependency
 * @return true if added successfully, false otherwise
 * @note AI-guidance: Called during Launch Go/No-Go process
 */
bool add_dependency_from_launch(int subsystem_id, const char* dependency_name);

/**
 * @brief Legacy function - will be deprecated
 * @note AI-guidance: Use register_subsystem_from_launch instead
 */
void register_standard_subsystems(void);

/**
 * @brief Update the registry when a subsystem is started
 * @param subsystem_name Name of the subsystem
 * @param success Whether the subsystem started successfully
 * @note AI-guidance: Called after subsystem init function
 */
void update_subsystem_on_startup(const char* subsystem_name, bool success);

/**
 * @brief Update the registry with all started subsystems
 * @note AI-guidance: Synchronizes registry with system state
 */
void update_registry_on_startup(void);

/**
 * @brief Update the registry when a subsystem is stopping
 * @param subsystem_name Name of the subsystem
 * @note AI-guidance: Called before subsystem shutdown
 */
void update_subsystem_on_shutdown(const char* subsystem_name);

/**
 * @brief Update the registry after a subsystem has stopped
 * @param subsystem_name Name of the subsystem
 * @note AI-guidance: Called after subsystem shutdown
 */
void update_subsystem_after_shutdown(const char* subsystem_name);

/**
 * @brief Update the registry during shutdown
 * @note AI-guidance: Marks all subsystems as stopping/inactive
 */
void update_registry_on_shutdown(void);

/**
 * @brief Get status of all running subsystems
 * @param status_buffer Pointer to char* for status buffer
 * @return true if successful, false otherwise
 * @note AI-guidance: Caller must free returned buffer
 */
bool get_running_subsystems_status(char** status_buffer);

/**
 * @brief Stop all subsystems in dependency-aware order
 * @return Number of subsystems stopped
 * @note AI-guidance: Used during shutdown sequence to safely stop subsystems
 */
size_t stop_all_subsystems_in_dependency_order(void);
// External declarations from state.h
extern AppConfig* app_config;
extern ServiceThreads logging_threads;
extern ServiceThreads webserver_threads;
extern ServiceThreads websocket_threads;
extern ServiceThreads mdns_server_threads;
extern ServiceThreads print_threads;

// Component shutdown flags
extern volatile sig_atomic_t mdns_client_system_shutdown;
extern volatile sig_atomic_t mail_relay_system_shutdown;
extern volatile sig_atomic_t swagger_system_shutdown;
extern volatile sig_atomic_t terminal_system_shutdown;

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

extern int init_mail_relay_subsystem(void);
extern void shutdown_mail_relay(void);

extern int init_swagger_subsystem(void);
extern void shutdown_swagger(void);

extern int init_terminal_subsystem(void);
extern void shutdown_terminal(void);

extern int init_print_subsystem(void);
extern void shutdown_print_queue(void);

/**
 * @brief Stop a subsystem and all its dependents safely
 * @param subsystem_id ID of the subsystem to stop
 * @return true if stopped successfully, false otherwise  
 * @note AI-guidance: Stops dependents first, then the subsystem itself
 */
bool stop_subsystem_and_dependents(int subsystem_id);

#endif /* REGISTRY_INTEGRATION_H */
