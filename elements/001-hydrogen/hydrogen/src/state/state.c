/*
 * State Management
 * 
 */

 // Global includes
#include <src/hydrogen.h>
#include <stdint.h>  // For uintptr_t

// Local includes
#include "state.h"
#include "state_types.h"

// Memory management implementation
void free_readiness_messages(LaunchReadiness* readiness) {
    if (readiness && readiness->messages) {
        for (size_t i = 0; readiness->messages[i]; i++) {
            // Only free if the pointer looks valid (not NULL and not in low memory)
            if (readiness->messages[i] && (uintptr_t)readiness->messages[i] > 0x1000) {
                free((void*)readiness->messages[i]);
            }
        }
        free(readiness->messages);
        readiness->messages = NULL;
    }
}

// Core state flags
volatile sig_atomic_t server_starting = 1;  // Start as true, will be set to false once startup complete
volatile sig_atomic_t server_running = 0;
volatile sig_atomic_t server_stopping = 0;
volatile sig_atomic_t restart_requested = 0;
volatile sig_atomic_t handler_flags_reset_needed = 0;  // For signal handler reset tracking
volatile int restart_count = 0;

pthread_cond_t terminate_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t terminate_mutex = PTHREAD_MUTEX_INITIALIZER;

// Component-specific shutdown flags with dependency awareness

volatile sig_atomic_t log_queue_shutdown = 0;
volatile sig_atomic_t web_server_shutdown = 0;
volatile sig_atomic_t websocket_server_shutdown = 0;
volatile sig_atomic_t mdns_server_system_shutdown = 0;
volatile sig_atomic_t mdns_client_system_shutdown = 0;
volatile sig_atomic_t mail_relay_system_shutdown = 0;
volatile sig_atomic_t swagger_system_shutdown = 0;
volatile sig_atomic_t terminal_system_shutdown = 0;
volatile sig_atomic_t print_system_shutdown = 0;
volatile sig_atomic_t print_queue_shutdown = 0;

// System thread handles with lifecycle management

pthread_t log_thread;
pthread_t webserver_thread;
pthread_t websocket_thread;
pthread_t mdns_server_thread;
pthread_t print_queue_thread;

// Thread tracking structures

ServiceThreads logging_threads;
ServiceThreads webserver_threads;
ServiceThreads websocket_threads;
ServiceThreads mdns_server_threads;
ServiceThreads print_threads;
ServiceThreads database_threads;

// Shared resource handles
mdns_server_t *mdns_server = NULL;
network_info_t *net_info = NULL;

// Signal handling
void signal_handler(int sig) {
    switch (sig) {
        case SIGHUP:
            // Set restart flags before initiating shutdown
            restart_requested = 1;
            restart_count++;
            handler_flags_reset_needed = 1;  // Ensure handlers are reset after restart
            log_this(SR_RESTART, "SIGHUP received, initiating restart", LOG_LEVEL_STATE, 0);
            log_this(SR_RESTART, "Restart count: %d", LOG_LEVEL_STATE, 1, restart_count);
            __sync_synchronize();  // Memory barrier to ensure flag visibility
            
            // Reset signal handlers immediately
            signal(SIGHUP, signal_handler);
            signal(SIGINT, signal_handler);
            signal(SIGTERM, signal_handler);
            
            graceful_shutdown();  // This will trigger landing and restart
            break;
        case SIGINT:
        case SIGTERM:
            // Record shutdown start time immediately when signal received
            record_shutdown_start_time();
            
            log_this(SR_SHUTDOWN, "%s received, initiating shutdown", LOG_LEVEL_STATE, 1, sig == SIGINT ? "SIGINT" : "SIGTERM");
            
            // Set signal-based shutdown flag for rapid exit
            __sync_bool_compare_and_swap(&signal_based_shutdown, 0, 1);
            __sync_bool_compare_and_swap(&server_running, 1, 0);
            __sync_bool_compare_and_swap(&server_stopping, 0, 1);
            __sync_synchronize();
            
            // Wake up the main event loop immediately
            pthread_cond_signal(&terminate_cond);
            break;
    }
}

// Track shutdown state
static volatile sig_atomic_t shutdown_in_progress = 0;
// Track if we're in a signal-based shutdown (SIGINT/SIGTERM) for rapid exit
volatile sig_atomic_t signal_based_shutdown = 0;

// Reset shutdown state - called after successful restart
void reset_shutdown_state(void) {
    __sync_bool_compare_and_swap(&shutdown_in_progress, 1, 0);
    __sync_synchronize();
}

// Graceful shutdown handling
void graceful_shutdown(void) {
    // Prevent multiple shutdown sequences
    if (!__sync_bool_compare_and_swap(&shutdown_in_progress, 0, 1)) {
        return;  // Shutdown already in progress
    }

    // Start shutdown sequence
    const char* subsystem = restart_requested ? SR_RESTART : SR_SHUTDOWN;
    log_this(subsystem, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(subsystem, restart_requested ?
        "Initiating restart sequence" :
        "Initiating shutdown sequence", LOG_LEVEL_STATE, 0);

    // Trigger landing sequence
    bool landing_ok = check_all_landing_readiness();
    
    // If landing failed and this was a restart attempt, reset flags
    if (!landing_ok && restart_requested) {
        __sync_bool_compare_and_swap(&restart_requested, 1, 0);
        reset_shutdown_state();
        __sync_synchronize();
    }
}
