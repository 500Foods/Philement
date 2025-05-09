// Define POSIX version before any includes
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

/*
 * Signal Handling for Hydrogen Shutdown
 * 
 * This module handles various signals (SIGINT, SIGTERM, SIGHUP) and
 * initiates the appropriate shutdown or restart actions.
 */

// Core system headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <features.h>

// Project headers
#include "shutdown_internal.h"
#include "../../logging/logging.h"
#include "../../landing/landing.h"

// Signal-related state flags
volatile sig_atomic_t restart_requested = 0;
volatile sig_atomic_t restart_count = 0;
volatile sig_atomic_t handler_flags_reset_needed = 0;

// Signal handler implementing graceful shutdown and restart initiation
//
// Design choices for signal handling:
// 1. Thread Safety
//    - Minimal work in signal context
//    - Atomic flag modifications only
//    - Deferred cleanup to main thread
//
// 2. Coordination
//    - Single point of shutdown/restart initiation
//    - Broadcast notification to all threads
//    - Prevents multiple shutdown attempts
//
// 3. Signal Types
//    - SIGINT (Ctrl+C): Clean shutdown
//    - SIGTERM: Clean shutdown (identical to SIGINT)
//    - SIGHUP: Restart with config reload (supports multiple restarts)

void signal_handler(int signum) {
    sigset_t mask, old_mask;
    sigfillset(&mask);
    sigprocmask(SIG_BLOCK, &mask, &old_mask);  // Block all signals during handler

    // Static flag to prevent multiple concurrent shutdowns/restarts
    static volatile sig_atomic_t already_shutting_down = 0;
    
    // Reset flags if marked from previous restart
    if (handler_flags_reset_needed) {
        already_shutting_down = 0;
        handler_flags_reset_needed = 0;
        log_this("Signal", "Signal handler flags reset for new operation", LOG_LEVEL_DEBUG);
    }
    
    // Only allow one shutdown/restart operation at a time
    if (!__sync_bool_compare_and_swap(&already_shutting_down, 0, 1)) {
        log_this("Signal", "Signal handling already in progress, ignoring %s",
                 LOG_LEVEL_DEBUG, signum == SIGHUP ? "SIGHUP" :
                               signum == SIGINT ? "SIGINT" : 
                               signum == SIGTERM ? "SIGTERM" : "UNKNOWN");
        sigprocmask(SIG_SETMASK, &old_mask, NULL);  // Restore signal mask
        return;  // Already handling shutdown
    }

    printf("\n");  // Keep newline for visual separation
    fflush(stdout);  // Ensure output is written

    // Handle different signal types with proper signal masking
    switch (signum) {
        case SIGHUP:
            // Use landing system's SIGHUP handler
            handle_sighup();
            graceful_shutdown();  // This will handle restart after cleanup
            break;

        case SIGINT:
            // Use landing system's SIGINT handler
            handle_sigint();
            graceful_shutdown();  // This will handle shutdown
            break;

        case SIGTERM:
            log_this("Signal", "SIGTERM received, initiating shutdown", LOG_LEVEL_STATE);
            // Set server state flags to prevent reinitialization during shutdown
            server_running = 0;
            server_stopping = 1;
            graceful_shutdown();
            break;

        default:
            log_this("Signal", "Unexpected signal %d, treating as shutdown", LOG_LEVEL_ALERT, signum);
            server_running = 0;
            server_stopping = 1;
            graceful_shutdown();
            break;
    }
}