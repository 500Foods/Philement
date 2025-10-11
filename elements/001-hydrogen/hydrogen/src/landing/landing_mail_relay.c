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
 * Note: System is currently under development
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include <src/landing/landing.h>

// External declarations
extern volatile sig_atomic_t mail_relay_system_shutdown;

// Check if the mail relay subsystem is ready to land
LaunchReadiness check_mail_relay_landing_readiness(void) {
    LaunchReadiness readiness = {0};

    // Set subsystem name
    readiness.subsystem = SR_MAIL_RELAY;

    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(5 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    int msg_count = 0;

    // Add the subsystem name as the first message
    readiness.messages[msg_count++] = strdup("Mail Relay");
    
    // Since system is under development, always ready to land
    readiness.messages[msg_count++] = strdup("  Go:      System under development");
    
    // Check for dependent subsystems
    // Only Print comes after Mail Relay, and it doesn't depend on it
    readiness.messages[msg_count++] = strdup("  Go:      No dependent subsystems");
    
    // All checks passed
    readiness.messages[msg_count++] = strdup("  Decide:  Go For Landing of Mail Relay");
    readiness.messages[msg_count] = NULL;
    readiness.ready = true;
    
    return readiness;
}

// Land the mail relay subsystem
int land_mail_relay_subsystem(void) {
    log_this(SR_MAIL_RELAY, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_MAIL_RELAY, "LANDING: " SR_MAIL_RELAY, LOG_LEVEL_STATE, 0);

    // Set shutdown flag
    mail_relay_system_shutdown = 1;
    
    // Since system is under development, always return success
    return 1;
}
