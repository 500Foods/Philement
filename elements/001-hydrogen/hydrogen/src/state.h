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

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <signal.h>
#include <pthread.h>

// Project headers
#include "configuration.h"
#include "mdns_server.h"
#include "network.h"
#include "utils_threads.h"

// Thread tracking structures
extern ServiceThreads logging_threads;
extern ServiceThreads web_threads;
extern ServiceThreads websocket_threads;
extern ServiceThreads mdns_threads;
extern ServiceThreads print_threads;

// Application state flags
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_starting;
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

// Component shutdown flags
extern volatile sig_atomic_t web_server_shutdown;
extern volatile sig_atomic_t print_queue_shutdown;
extern volatile sig_atomic_t log_queue_shutdown;
extern volatile sig_atomic_t mdns_server_shutdown;
extern volatile sig_atomic_t websocket_server_shutdown;

// Queue Threads
extern pthread_t log_thread;
extern pthread_t print_queue_thread;
extern pthread_t mdns_thread;
extern pthread_t web_thread;
extern pthread_t websocket_thread;

// Shared resources
extern AppConfig *app_config;
extern mdns_t *mdns;
extern network_info_t *net_info;

#endif // HYDROGEN_STATE_H