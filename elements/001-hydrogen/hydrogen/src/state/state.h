/*
 * State Management for Safety-Critical 3D Printing
 * 
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
#include "../threads/threads.h"        // Thread management subsystem
#include "../registry/registry.h"      // Registry subsystem
#include "../landing/landing.h"        // For check_all_landing_readiness


// Application state flags
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t restart_requested;
extern volatile sig_atomic_t handler_flags_reset_needed;  // For signal handler reset tracking
extern volatile sig_atomic_t signal_based_shutdown;      // For rapid exit during SIGINT/SIGTERM
extern volatile int restart_count;

// Thread synchronization
extern pthread_cond_t terminate_cond;
extern pthread_mutex_t terminate_mutex;

// Thread tracking structures
extern ServiceThreads logging_threads;
extern ServiceThreads webserver_threads;
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
extern pthread_t webserver_thread;
extern pthread_t websocket_thread;
extern pthread_t mdns_server_thread;
extern pthread_t print_queue_thread;

// Shared resources
extern AppConfig *app_config;
extern mdns_server_t *mdns_server;
extern network_info_t *net_info;

// Core system functions
void graceful_shutdown(void);
void signal_handler(int sig);
void reset_shutdown_state(void);  // Reset shutdown state after restart

// Registry integration functions
void register_standard_subsystems(void);
void update_registry_on_startup(void);
void update_registry_on_shutdown(void);
bool get_running_subsystems_status(char** status_buffer);

#endif // HYDROGEN_STATE_H
