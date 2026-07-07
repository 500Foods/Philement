/*
 * Launch Mail Relay Subsystem
 * 
 * This module handles the initialization of the mail relay subsystem.
 * It provides functions for checking readiness and launching the mail relay.
 * 
 * Dependencies:
 * - Network subsystem must be initialized and ready
 * - Database subsystem is recommended when persistence is enabled
 * 
 * Note: Shutdown functionality has been moved to landing/landing_mail_relay.c
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "launch.h"

// Mail relay includes
#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_message.h>

// External declarations
extern ServiceThreads mailrelay_threads;
extern volatile sig_atomic_t mail_relay_system_shutdown;

// Registry ID for the Mail Relay subsystem
int mailrelay_subsystem_id = -1;

// Helper function to validate and report range errors for size_t values
static bool validate_int_range(int value, int min, int max, 
                               const char* field_name, const char*** messages, 
                               size_t* count, size_t* capacity) {
    if (value < min || value > max) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Invalid %s %d (must be between %d and %d)",
                field_name, value, min, max);
        add_launch_message(messages, count, capacity, strdup(msg));
        return false;
    }
    return true;
}

// Check if the mail relay subsystem is ready to launch
LaunchReadiness check_mail_relay_launch_readiness(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool is_ready = true;

    // First message is subsystem name
    add_launch_message(&messages, &count, &capacity, strdup(SR_MAIL_RELAY));

    // Register the Mail Relay subsystem so the launch loop can resolve its ID
    // and dispatch launch_mail_relay_subsystem(). Threads/init/shutdown are
    // managed by the launch/landing handlers and the lifecycle helpers.
    if (mailrelay_subsystem_id < 0) {
        mailrelay_subsystem_id = register_subsystem(SR_MAIL_RELAY, NULL, NULL, NULL, NULL, NULL);
    }

    // Register dependency on Network subsystem
    int relay_id = get_subsystem_id_by_name(SR_MAIL_RELAY);
    if (relay_id >= 0) {
        if (!add_dependency_from_launch(relay_id, SR_NETWORK)) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Failed to register Network dependency"));
            finalize_launch_messages(&messages, &count, &capacity);
            return (LaunchReadiness){ .subsystem = SR_MAIL_RELAY, .ready = false, .messages = messages };
        }
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Network dependency registered"));
    }

    // Check basic configuration
    if (!app_config) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Configuration not available"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_MAIL_RELAY, .ready = false, .messages = messages };
    }

    const MailRelayConfig* config = &app_config->mail_relay;

    if (!config->Enabled) {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Mail relay disabled - clean skip"));
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  Skip Launch of Mail Relay Subsystem"));
        finalize_launch_messages(&messages, &count, &capacity);
        return (LaunchReadiness){ .subsystem = SR_MAIL_RELAY, .ready = true, .messages = messages };
    }
    add_launch_message(&messages, &count, &capacity, strdup("  Go:      Mail relay enabled in configuration"));

    // Validate port range
    if (!validate_int_range(config->ListenPort, 1, 65535, "listen port", &messages, &count, &capacity)) {
        is_ready = false;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Listen port valid"));
    }

    // If inbound enabled, validate listen port is not privileged on restricted systems
    if (config->InboundEnabled && config->ListenPort < 1024) {
        char msg[128];
        snprintf(msg, sizeof(msg), "  No-Go:   Inbound listener on privileged port %d may require root", config->ListenPort);
        add_launch_message(&messages, &count, &capacity, strdup(msg));
        is_ready = false;
    }

    // Validate worker count
    if (config->Workers <= 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid worker count (must be positive)"));
        is_ready = false;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Worker count valid"));
    }

    // Outbound validation
    if (config->OutboundEnabled) {
        if (config->OutboundServerCount <= 0 || config->OutboundServerCount > MAX_OUTBOUND_SERVERS) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Outbound enabled but no servers configured"));
            is_ready = false;
        } else {
            bool any_server_valid = false;
            for (int i = 0; i < config->OutboundServerCount; i++) {
                bool has_host = config->Servers[i].Host && config->Servers[i].Host[0] != '\0';
                bool has_port = config->Servers[i].Port && config->Servers[i].Port[0] != '\0';
                bool has_creds = config->Servers[i].Username && config->Servers[i].Username[0] != '\0' &&
                                 config->Servers[i].Password && config->Servers[i].Password[0] != '\0';
                bool needs_auth = config->Servers[i].AuthMode != MAIL_AUTH_MODE_NONE;

                if (has_host && has_port && (!needs_auth || has_creds)) {
                    any_server_valid = true;
                    char msg[128];
                    snprintf(msg, sizeof(msg), "  Go:      Server %d configuration valid", i + 1);
                    add_launch_message(&messages, &count, &capacity, strdup(msg));
                } else {
                    char msg[256];
                    if (!has_host || !has_port) {
                        snprintf(msg, sizeof(msg), "  No-Go:   Server %d missing host or port", i + 1);
                    } else if (needs_auth && !has_creds) {
                        snprintf(msg, sizeof(msg), "  No-Go:   Server %d missing credentials for AuthMode %d", i + 1, config->Servers[i].AuthMode);
                    } else {
                        snprintf(msg, sizeof(msg), "  No-Go:   Server %d configuration invalid", i + 1);
                    }
                    add_launch_message(&messages, &count, &capacity, strdup(msg));
                    is_ready = false;
                }
            }
            if (!any_server_valid) {
                add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   No valid outbound servers available"));
                is_ready = false;
            }
        }
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Outbound sending disabled"));
    }

    // Queue persistence validation
    if (config->Queue.Persist) {
        if (!config->Database || !config->Database[0]) {
            add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Queue persistence enabled but no database configured"));
            is_ready = false;
        } else {
            add_launch_message(&messages, &count, &capacity, strdup("  Go:      Queue persistence has database target"));
        }
    }

    // Validate queue settings
    if (config->Queue.MaxInMemory <= 0 && config->Queue.MaxQueueSize <= 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid queue size (must be positive)"));
        is_ready = false;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Queue settings valid"));
    }

    if (config->Queue.RetryAttempts < 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid retry attempts (must be non-negative)"));
        is_ready = false;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Retry attempts valid"));
    }

    if (config->Queue.RetryDelaySeconds <= 0) {
        add_launch_message(&messages, &count, &capacity, strdup("  No-Go:   Invalid retry delay (must be positive)"));
        is_ready = false;
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Go:      Retry delay valid"));
    }

    // Final decision
    if (is_ready) {
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  Go For Launch of Mail Relay Subsystem"));
    } else {
        add_launch_message(&messages, &count, &capacity, strdup("  Decide:  No-Go For Launch of Mail Relay Subsystem"));
    }

    finalize_launch_messages(&messages, &count, &capacity);

    return (LaunchReadiness){
        .subsystem = SR_MAIL_RELAY,
        .ready = is_ready,
        .messages = messages
    };
}

// Launch the mail relay subsystem
int launch_mail_relay_subsystem(void) {
    
    mail_relay_system_shutdown = 0;
    
    log_this(SR_MAIL_RELAY, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_MAIL_RELAY, "LAUNCH: " SR_MAIL_RELAY, LOG_LEVEL_STATE, 0);
    // Initialize mail relay thread tracking structure
    init_service_threads(&mailrelay_threads, SR_MAIL_RELAY);

    // SendRawOnLaunch smoke test: fire one message synchronously if configured.
    // This is test-only and must not allocate workers or open queues.
    const MailRelayConfig* config = app_config ? &app_config->mail_relay : NULL;
    if (config && config->Test.SendRawOnLaunch && config->OutboundServerCount > 0) {
        log_this(SR_MAIL_RELAY,
                 "TEST: SendRawOnLaunch requested, sending smoke test via server 0",
                 LOG_LEVEL_STATE, 0);

        MailRelayMessage msg = {0};
        mailrelay_message_init(&msg);
        msg.from = config->Test.TestFrom;
        if (!msg.from) msg.from = config->DefaultFrom;
        msg.to[0] = config->Test.TestTo;
        if (msg.to[0]) msg.to_count = 1;
        msg.subject = config->Test.TestSubject;
        msg.text_body = config->Test.TestBody;

        MailRelayResult result = {0};
        mailrelay_result_init(&result);
        const char* app_name = (app_config && app_config->server.server_name)
                               ? app_config->server.server_name : "hydrogen";

        bool sent = mailrelay_send_raw(&msg,
                                       &config->Servers[0],
                                       config->DefaultFrom,
                                       app_name,
                                       &result);

        if (sent) {
            log_this(SR_MAIL_RELAY,
                     "TEST: SendRawOnLaunch success code=%d text='%s' duration=%.1fms",
                     LOG_LEVEL_STATE, 3,
                     result.smtp_code,
                     result.smtp_text,
                     result.duration_ms);
        } else {
            log_this(SR_MAIL_RELAY,
                     "TEST: SendRawOnLaunch failed code=%d error='%s' duration=%.1fms",
                     LOG_LEVEL_STATE, 3,
                     result.smtp_code,
                     result.error[0] ? result.error : result.smtp_text,
                     result.duration_ms);
        }
    }
    
    // TODO: Add proper initialization when system is ready (Phase 3: workers, queue)
    return 1;
}
