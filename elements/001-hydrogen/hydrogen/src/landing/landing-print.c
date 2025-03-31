/*
 * Landing Print Subsystem
 * 
 * This module handles the landing (shutdown) sequence for the print subsystem.
 * It provides functions for:
 * - Checking print queue landing readiness
 * - Managing print thread shutdown
 * - Cleaning up print resources
 * 
 * The print subsystem shutdown handles:
 * - Print job queue cleanup
 * - Print thread termination
 * - Print resource deallocation
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include "landing.h"
#include "landing_readiness.h"
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../threads/threads.h"
#include "../state/registry/subsystem_registry.h"
#include "../state/registry/subsystem_registry_integration.h"

// External declarations
extern ServiceThreads print_threads;
extern pthread_t print_queue_thread;
extern volatile sig_atomic_t print_system_shutdown;

// Check if the print subsystem is ready to land
LandingReadiness check_print_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "Print Queue";
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(5 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup("Print Queue");
    
    // Check if print system is actually running
    if (!is_subsystem_running_by_name("PrintQueue")) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   Print Queue not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Print Queue");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Check thread status
    bool threads_ready = true;
    if (print_queue_thread && print_threads.thread_count > 0) {
        readiness.messages[1] = strdup("  Go:      Print Queue thread ready for shutdown");
    } else {
        threads_ready = false;
        readiness.messages[1] = strdup("  No-Go:   Print Queue thread not accessible");
    }
    
    // Final decision
    if (threads_ready) {
        readiness.ready = true;
        readiness.messages[2] = strdup("  Go:      All resources ready for cleanup");
        readiness.messages[3] = strdup("  Decide:  Go For Landing of Print Queue");
    } else {
        readiness.ready = false;
        readiness.messages[2] = strdup("  No-Go:   Resources not ready for cleanup");
        readiness.messages[3] = strdup("  Decide:  No-Go For Landing of Print Queue");
    }
    readiness.messages[4] = NULL;
    
    return readiness;
}

// Forward declaration of core implementation from print_queue_manager.c
extern void shutdown_print_queue(void);