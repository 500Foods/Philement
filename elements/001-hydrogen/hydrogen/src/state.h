/*
 * Shared state management for the Hydrogen 3D printer server.
 * 
 * This file defines the global state variables used across different components
 * of the server, including thread handles, synchronization primitives, and shared
 * resources. It provides a centralized point for state management to ensure
 * thread-safe access and proper resource lifecycle.
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

// Application state flags
extern volatile sig_atomic_t keep_running;
extern volatile sig_atomic_t shutting_down;
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