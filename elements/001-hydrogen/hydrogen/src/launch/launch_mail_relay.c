/*
 * Launch Mail Relay Subsystem
 * 
 * This module handles the initialization of the mail relay subsystem.
 * It provides functions for checking readiness and launching the mail relay.
 * 
 * Dependencies:
 * - Network subsystem must be initialized and ready
 * 
 * Note: Shutdown functionality has been moved to landing/landing_mail_relay.c
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "launch.h"
#include "../utils/utils_logging.h"
#include "../threads/threads.h"
#include "../config/config.h"
#include "../registry/registry_integration.h"

// External declarations
extern volatile sig_atomic_t mail_relay_system_shutdown;
extern AppConfig* app_config;

// Check if the mail relay subsystem is ready to launch
LaunchReadiness check_mail_relay_launch_readiness(void) {
    LaunchReadiness readiness = {0};
    
    // Calculate maximum possible messages:
    // Base messages (subsystem, network, config, port, workers, queue settings) = ~12
    // Per server: 1 success + 4 possible errors = 5 messages per server
    // Final decision message = 1
    const int base_messages = 12;
    const int messages_per_server = 5;
    const int max_messages = base_messages + (MAX_OUTBOUND_SERVERS * messages_per_server) + 1;
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc((max_messages + 1) * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    int msg_count = 0;
    
    // Safety check for message count
    #define CHECK_MSG_COUNT() \
        if (msg_count >= max_messages) { \
            readiness.messages[msg_count] = NULL; \
            readiness.ready = false; \
            return readiness; \
        }
    
    // Add the subsystem name as the first message
    CHECK_MSG_COUNT();
    readiness.messages[msg_count++] = strdup("Mail Relay");
    
    // Register dependency on Network subsystem
    int relay_id = get_subsystem_id_by_name("Mail Relay");
    if (relay_id >= 0) {
        if (!add_dependency_from_launch(relay_id, "Network")) {
            readiness.messages[msg_count++] = strdup("  No-Go:   Failed to register Network dependency");
            readiness.messages[msg_count] = NULL;
            readiness.ready = false;
            return readiness;
        }
        CHECK_MSG_COUNT();
        readiness.messages[msg_count++] = strdup("  Go:      Network dependency registered");
        
        // Verify Network subsystem is running
        if (!is_subsystem_running_by_name("Network")) {
            readiness.messages[msg_count++] = strdup("  No-Go:   Network subsystem not running");
            readiness.messages[msg_count] = NULL;
            readiness.ready = false;
            return readiness;
        }
        CHECK_MSG_COUNT();
        readiness.messages[msg_count++] = strdup("  Go:      Network subsystem running");
    }
    
    // Check basic configuration
    if (!app_config || !app_config->mail_relay.Enabled) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Mail relay disabled in configuration");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    CHECK_MSG_COUNT();
    readiness.messages[msg_count++] = strdup("  Go:      Mail relay enabled in configuration");

    // Validate configuration values
    const MailRelayConfig* config = &app_config->mail_relay;

    // Validate port range
    if (config->ListenPort <= 0 || config->ListenPort > 65535) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Invalid listen port (must be 1-65535)");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    CHECK_MSG_COUNT();
    readiness.messages[msg_count++] = strdup("  Go:      Listen port valid");

    // Validate worker count
    if (config->Workers <= 0) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Invalid worker count (must be positive)");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    CHECK_MSG_COUNT();
    readiness.messages[msg_count++] = strdup("  Go:      Worker count valid");

    // Validate queue settings
    if (config->Queue.MaxQueueSize <= 0) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Invalid max queue size (must be positive)");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    CHECK_MSG_COUNT();
    readiness.messages[msg_count++] = strdup("  Go:      Queue size valid");

    if (config->Queue.RetryAttempts < 0) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Invalid retry attempts (must be non-negative)");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    CHECK_MSG_COUNT();
    readiness.messages[msg_count++] = strdup("  Go:      Retry attempts valid");

    if (config->Queue.RetryDelaySeconds <= 0) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Invalid retry delay (must be positive)");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    CHECK_MSG_COUNT();
    readiness.messages[msg_count++] = strdup("  Go:      Retry delay valid");

    // Must have at least one outbound server
    if (config->OutboundServerCount <= 0 || config->OutboundServerCount > MAX_OUTBOUND_SERVERS) {
        readiness.messages[msg_count++] = strdup("  No-Go:   Invalid number of outbound servers");
        readiness.messages[msg_count] = NULL;
        readiness.ready = false;
        return readiness;
    }
    CHECK_MSG_COUNT();
    readiness.messages[msg_count++] = strdup("  Go:      Server count valid");

    // Validate each configured server
    for (int i = 0; i < config->OutboundServerCount; i++) {
        char msg_buffer[256];

        if (!config->Servers[i].Host || !config->Servers[i].Host[0]) {
            snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Server %d missing host", i + 1);
            readiness.messages[msg_count++] = strdup(msg_buffer);
            readiness.messages[msg_count] = NULL;
            readiness.ready = false;
            return readiness;
        }

        if (!config->Servers[i].Port || !config->Servers[i].Port[0]) {
            snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Server %d missing port", i + 1);
            readiness.messages[msg_count++] = strdup(msg_buffer);
            readiness.messages[msg_count] = NULL;
            readiness.ready = false;
            return readiness;
        }

        if (!config->Servers[i].Username || !config->Servers[i].Username[0]) {
            snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Server %d missing username", i + 1);
            readiness.messages[msg_count++] = strdup(msg_buffer);
            readiness.messages[msg_count] = NULL;
            readiness.ready = false;
            return readiness;
        }

        if (!config->Servers[i].Password || !config->Servers[i].Password[0]) {
            snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Server %d missing password", i + 1);
            readiness.messages[msg_count++] = strdup(msg_buffer);
            readiness.messages[msg_count] = NULL;
            readiness.ready = false;
            return readiness;
        }

        CHECK_MSG_COUNT();
        snprintf(msg_buffer, sizeof(msg_buffer), "  Go:      Server %d configuration valid", i + 1);
        readiness.messages[msg_count++] = strdup(msg_buffer);
    }

    // All checks passed
    CHECK_MSG_COUNT();
    readiness.messages[msg_count++] = strdup("  Decide:  Go For Launch of Mail Relay Subsystem");
    readiness.messages[msg_count] = NULL;
    readiness.ready = true;
    
    return readiness;
}

// Launch the mail relay subsystem
int launch_mail_relay_subsystem(void) {
    // Reset shutdown flag
    mail_relay_system_shutdown = 0;
    
    // Initialize mail relay system
    // TODO: Add proper initialization when system is ready
    return 1;
}
