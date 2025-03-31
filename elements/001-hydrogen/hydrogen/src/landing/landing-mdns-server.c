/*
 * Landing mDNS Server Subsystem
 * 
 * This module handles the landing (shutdown) sequence for the mDNS server subsystem.
 * It provides functions for:
 * - Checking mDNS server landing readiness
 * - Managing network service shutdown
 * - Cleaning up mDNS server threads
 * 
 * Dependencies:
 * - Must coordinate with Network subsystem for clean shutdown
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
#include "../registry/registry.h"
#include "../registry/registry_integration.h"
#include "../state/state_types.h"

// External declarations
extern ServiceThreads mdns_server_threads;
extern volatile sig_atomic_t mdns_server_system_shutdown;

// Check if the mDNS server subsystem is ready to land
LaunchReadiness check_mdns_server_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = "mDNS Server";
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(6 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup("mDNS Server");
    
    // Check if mDNS server is actually running
    if (!is_subsystem_running_by_name("mDNSServer")) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   mDNS Server not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of mDNS Server");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Check Network subsystem status
    bool network_ready = is_subsystem_running_by_name("Network");
    if (!network_ready) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   Network subsystem not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of mDNS Server");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Check thread status
    bool threads_ready = true;
    if (mdns_server_threads.thread_count > 0) {
        readiness.messages[1] = strdup("  Go:      mDNS Server threads ready for shutdown");
        char thread_msg[128];
        snprintf(thread_msg, sizeof(thread_msg), "  Go:      Active threads: %d", 
                 mdns_server_threads.thread_count);
        readiness.messages[2] = strdup(thread_msg);
    } else {
        threads_ready = false;
        readiness.messages[1] = strdup("  No-Go:   mDNS Server threads not accessible");
    }
    
    // Final decision
    if (threads_ready && network_ready) {
        readiness.ready = true;
        readiness.messages[3] = strdup("  Go:      Network subsystem ready");
        readiness.messages[4] = strdup("  Decide:  Go For Landing of mDNS Server");
    } else {
        readiness.ready = false;
        readiness.messages[3] = strdup("  No-Go:   Resources not ready for cleanup");
        readiness.messages[4] = strdup("  Decide:  No-Go For Landing of mDNS Server");
    }
    readiness.messages[5] = NULL;
    
    return readiness;
}

// Land the mDNS server subsystem
int land_mdns_server_subsystem(void) {
    log_this("mDNS Server", "Beginning mDNS Server shutdown sequence", LOG_LEVEL_STATE);
    
    // Signal thread shutdown
    mdns_server_system_shutdown = 1;
    log_this("mDNS Server", "Signaled mDNS Server threads to stop", LOG_LEVEL_STATE);
    
    // Log thread count before cleanup
    log_this("mDNS Server", "Cleaning up %d mDNS Server threads", LOG_LEVEL_STATE, 
             mdns_server_threads.thread_count);
    
    // Remove all mDNS server threads from tracking
    for (int i = 0; i < mdns_server_threads.thread_count; i++) {
        remove_service_thread(&mdns_server_threads, mdns_server_threads.thread_ids[i]);
    }
    
    // Reinitialize thread structure
    init_service_threads(&mdns_server_threads);
    
    log_this("mDNS Server", "mDNS Server shutdown complete", LOG_LEVEL_STATE);
    return 1; // Success
}