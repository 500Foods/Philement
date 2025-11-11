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

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "launch.h"

// Check if the mail relay subsystem is ready to launch
LaunchReadiness check_mail_relay_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool ready = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup(SR_MAIL_RELAY));

    // Register dependency on Network subsystem
    int relay_id = get_subsystem_id_by_name(SR_MAIL_RELAY);
    if (relay_id >= 0) {
        if (!add_dependency_from_launch(relay_id, SR_NETWORK)) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register Network dependency"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = SR_MAIL_RELAY, .ready = false, .messages = messages };
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Network dependency registered"));

        // Verify Network subsystem is running
        if (!is_subsystem_running_by_name(SR_NETWORK)) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Network subsystem not running"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = SR_MAIL_RELAY, .ready = false, .messages = messages };
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Network subsystem running"));
    }

    // Check basic configuration
    if (!app_config || !app_config->mail_relay.Enabled) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Mail relay disabled in configuration"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_MAIL_RELAY, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Mail relay enabled in configuration"));

    // Validate configuration values
    const MailRelayConfig* config = &app_config->mail_relay;

    // Validate port range
    if (config->ListenPort <= 0 || config->ListenPort > 65535) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid listen port (must be 1-65535)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_MAIL_RELAY, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Listen port valid"));

    // Validate worker count
    if (config->Workers <= 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid worker count (must be positive)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_MAIL_RELAY, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Worker count valid"));

    // Validate queue settings
    if (config->Queue.MaxQueueSize <= 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid max queue size (must be positive)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_MAIL_RELAY, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Queue size valid"));

    if (config->Queue.RetryAttempts < 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid retry attempts (must be non-negative)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_MAIL_RELAY, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Retry attempts valid"));

    if (config->Queue.RetryDelaySeconds <= 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid retry delay (must be positive)"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_MAIL_RELAY, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Retry delay valid"));

    // Must have at least one outbound server
    if (config->OutboundServerCount <= 0 || config->OutboundServerCount > MAX_OUTBOUND_SERVERS) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid number of outbound servers"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_MAIL_RELAY, .ready = false, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Server count valid"));

    // Validate each configured server
    for (int i = 0; i < config->OutboundServerCount; i++) {
        char msg_buffer[256];

        if (!config->Servers[i].Host || !config->Servers[i].Host[0]) {
            snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Server %d missing host", i + 1);
            add_launch_message(&messages, &count, &capacity, strdup(msg_buffer));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = SR_MAIL_RELAY, .ready = false, .messages = messages };
        }

        if (!config->Servers[i].Port || !config->Servers[i].Port[0]) {
            snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Server %d missing port", i + 1);
            add_launch_message(&messages, &count, &capacity, strdup(msg_buffer));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = SR_MAIL_RELAY, .ready = false, .messages = messages };
        }

        if (!config->Servers[i].Username || !config->Servers[i].Username[0]) {
            snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Server %d missing username", i + 1);
            add_launch_message(&messages, &count, &capacity, strdup(msg_buffer));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = SR_MAIL_RELAY, .ready = false, .messages = messages };
        }

        if (!config->Servers[i].Password || !config->Servers[i].Password[0]) {
            snprintf(msg_buffer, sizeof(msg_buffer), "  No-Go:   Server %d missing password", i + 1);
            add_launch_message(&messages, &count, &capacity, strdup(msg_buffer));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = SR_MAIL_RELAY, .ready = false, .messages = messages };
        }

        snprintf(msg_buffer, sizeof(msg_buffer), "  Go:      Server %d configuration valid", i + 1);
        add_launch_message(&messages, &count, &capacity, strdup(msg_buffer));
    }

    // All checks passed
    add_launch_message(&messages, &count, &capacity, strdup("  Decide:  Go For Launch of Mail Relay Subsystem"));

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = SR_MAIL_RELAY,
        .ready = ready,
        .messages = messages
    };
}

// Launch the mail relay subsystem
int launch_mail_relay_subsystem(void) {
    
    mail_relay_system_shutdown = 0;
    
    log_this(SR_MAIL_RELAY, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_MAIL_RELAY, "LAUNCH: " SR_MAIL_RELAY, LOG_LEVEL_STATE, 0);
    
    // Initialize mail relay system
    // TODO: Add proper initialization when system is ready
    return 1;
}
