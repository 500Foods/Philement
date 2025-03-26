/*
 * Internal Header for Shutdown Component Integration
 * 
 * This header defines the internal interfaces between the various
 * shutdown-related components. It is not intended to be included
 * by external modules.
 */

#ifndef HYDROGEN_SHUTDOWN_INTERNAL_H
#define HYDROGEN_SHUTDOWN_INTERNAL_H

// Core system headers
#include <signal.h>
#include <pthread.h>
#include <stdbool.h>

// Project headers
#include "shutdown.h"
#include "../state.h"
#include "../registry/subsystem_registry.h"
#include "../registry/subsystem_registry_integration.h"
#include "../../utils/utils_time.h"
#include "../../config/launch/landing.h"

// Shared flags and state for shutdown coordination
extern volatile sig_atomic_t restart_requested;
extern volatile sig_atomic_t restart_count;
extern volatile sig_atomic_t handler_flags_reset_needed;

// Signal handling function - Defined in shutdown_signals.c
void signal_handler(int signum);

// Restart functions - Defined in shutdown_restart.c
int restart_hydrogen(const char* config_path);

// Subsystem shutdown function - Defined in subsystem_registry_integration.c
size_t stop_all_subsystems_in_dependency_order(void);

// Resource cleanup functions - Defined in shutdown_resources.c
void shutdown_network(void);
void free_app_config(void);

// Utility functions - Defined in shutdown.c
void log_final_shutdown_message(void);

#endif // HYDROGEN_SHUTDOWN_INTERNAL_H