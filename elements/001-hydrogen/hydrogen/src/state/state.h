/*
 * State Management for Safety-Critical 3D Printing
 * 
 * Why Centralized State Matters:
 * 1. Hardware Safety
 *    - Coordinated emergency stops
 *    - Temperature limit enforcement
 *    - Motion boundary protection
 *    - Power system monitoring
 * 
 * 2. Real-time Control
 *    Why This Design?
 *    - Immediate state updates
 *    - Synchronized movements
 *    - Thermal management
 *    - Timing precision
 * 
 * 3. Resource Coordination
 *    Why This Approach?
 *    - Thread synchronization
 *    - Memory management
 *    - File system access
 *    - Network resources
 * 
 * 4. Error Recovery
 *    Why These Features?
 *    - Print job preservation
 *    - Hardware protection
 *    - State restoration
 *    - Failure isolation
 * 
 * 5. System Monitoring
 *    Why This Matters?
 *    - Component health tracking
 *    - Resource utilization
 *    - Performance metrics
 *    - Diagnostic support
 */

#ifndef HYDROGEN_STATE_H
#define HYDROGEN_STATE_H

// Core system headers
#include <signal.h>
#include <pthread.h>

// Project headers
#include "../config/config_forward.h"  // For AppConfig type
#include "../config/config.h"          // For config functions
#include "../mdns/mdns_server.h"
#include "../network/network.h"
#include "../utils/utils_threads.h"
#include "registry/subsystem_registry.h"  // Subsystem registry


// Application state flags
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t server_stopping;

// Thread synchronization
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

// Thread tracking structures
extern ServiceThreads logging_threads;
extern ServiceThreads web_threads;
extern ServiceThreads websocket_threads;
extern ServiceThreads mdns_server_threads;
extern ServiceThreads print_threads;

// Component shutdown flags
extern volatile sig_atomic_t log_queue_shutdown;
extern volatile sig_atomic_t web_server_shutdown;
extern volatile sig_atomic_t websocket_server_shutdown;
extern volatile sig_atomic_t mdns_server_system_shutdown;
extern volatile sig_atomic_t mdns_client_system_shutdown;
extern volatile sig_atomic_t mail_relay_system_shutdown;
extern volatile sig_atomic_t swagger_system_shutdown;
extern volatile sig_atomic_t terminal_system_shutdown;
extern volatile sig_atomic_t print_system_shutdown;
extern volatile sig_atomic_t print_queue_shutdown;

// Queue Threads
extern pthread_t log_thread;
extern pthread_t web_thread;
extern pthread_t websocket_thread;
extern pthread_t mdns_server_thread;
extern pthread_t print_queue_thread;

// Shared resources
extern AppConfig *app_config;
extern mdns_server_t *mdns_server;
extern network_info_t *net_info;

// Core system functions
void graceful_shutdown(void);

// Subsystem registry integration functions
void register_standard_subsystems(void);
void update_subsystem_registry_on_startup(void);
void update_subsystem_registry_on_shutdown(void);
bool get_running_subsystems_status(char** status_buffer);

#endif // HYDROGEN_STATE_H