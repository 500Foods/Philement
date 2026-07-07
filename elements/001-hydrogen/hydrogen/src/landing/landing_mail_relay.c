/*
 * Landing Mail Relay Subsystem
 * 
 * This module handles the landing (shutdown) of the mail relay subsystem.
 * It provides functions for checking landing readiness and shutting down
 * the mail relay.
 * 
 * Dependencies:
 * - No subsystems depend on Mail Relay
 * 
 * Note: System is under active development
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "landing.h"

#include <src/landing/landing.h>
#include <src/utils/utils_logging.h>
#include <src/threads/threads.h>
#include <src/config/config.h>
#include <src/registry/registry_integration.h>
#include <src/state/state_types.h>

// External declarations
extern ServiceThreads mailrelay_threads;
extern volatile sig_atomic_t mail_relay_system_shutdown;

// Public lifecycle shutdown function from the mailrelay module
extern void mailrelay_shutdown(void);

// Check if the mail relay subsystem is ready to land
LaunchReadiness check_mail_relay_landing_readiness(void) {
    LaunchReadiness readiness = {0};
    readiness.subsystem = SR_MAIL_RELAY;

    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(10 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    int msg_count = 0;
    
    // Add the subsystem name as the first message
    readiness.messages[msg_count++] = strdup(SR_MAIL_RELAY);
    
    // Check if mail relay subsystem is running
    if (!is_subsystem_running_by_name(SR_MAIL_RELAY)) {
        readiness.messages[msg_count++] = strdup("  Go:      Mail relay not running");
        readiness.messages[msg_count++] = strdup("  Go:      No active workers to drain");
        readiness.messages[msg_count++] = strdup("  Go:      No dependent subsystems");
        readiness.messages[msg_count++] = strdup("  Decide:  Go For Landing of Mail Relay Subsystem");
        readiness.messages[msg_count] = NULL;
        readiness.ready = true;
        return readiness;
    }
    readiness.messages[msg_count++] = strdup("  Go:      Mail relay running");
    
    // Check for active worker threads
    if (mailrelay_threads.thread_count > 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  Go:      %d worker thread(s) to drain", mailrelay_threads.thread_count);
        readiness.messages[msg_count++] = strdup(msg);
    } else {
        readiness.messages[msg_count++] = strdup("  Go:      No active worker threads");
    }
    
    // Check for dependent subsystems
    // Print is the last subsystem, so it has no dependents to check
    readiness.messages[msg_count++] = strdup("  Go:      No dependent subsystems");
    
    // All checks passed
    readiness.messages[msg_count++] = strdup("  Decide:  Go For Landing of Mail Relay Subsystem");
    readiness.messages[msg_count] = NULL;
    readiness.ready = true;
    
    return readiness;
}

// Land the mail relay subsystem
int land_mail_relay_subsystem(void) {
    log_this(SR_MAIL_RELAY, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_MAIL_RELAY, "LANDING: " SR_MAIL_RELAY, LOG_LEVEL_DEBUG, 0);

    // Mark the subsystem as stopping in the registry before tearing down.
    update_subsystem_on_shutdown(SR_MAIL_RELAY);

    // Centralized shutdown: sets the shutdown flag, wakes workers, drains
    // tracked threads, and frees the runtime instance.
    mailrelay_shutdown();

    // Mark the subsystem as inactive now that workers are drained.
    update_subsystem_after_shutdown(SR_MAIL_RELAY);

    log_this(SR_MAIL_RELAY, "LANDING: " SR_MAIL_RELAY " COMPLETE", LOG_LEVEL_DEBUG, 0);

    return 1;
}
