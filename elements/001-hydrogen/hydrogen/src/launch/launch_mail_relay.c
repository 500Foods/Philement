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

// Registry integration
#include <src/registry/registry_integration.h>

// Mail relay includes
#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_workers.h>

// External declarations
extern ServiceThreads mailrelay_threads;
extern volatile sig_atomic_t mail_relay_system_shutdown;

// Public lifecycle function from the mailrelay module
extern bool mailrelay_init(void);

// Landing function referenced when registering with the registry.
extern int land_mail_relay_subsystem(void);

// Registry ID for the Mail Relay subsystem
int mailrelay_subsystem_id = -1;

// Register Mail Relay with the subsystem registry for launch/landing dispatch.
// Matches the mDNS server pattern so the registry knows our threads, shutdown
// flag, init function, and shutdown function.
static void register_mail_relay_for_launch(void) {
    if (mailrelay_subsystem_id < 0) {
        mailrelay_subsystem_id = register_subsystem_from_launch(
            SR_MAIL_RELAY,
            &mailrelay_threads,
            NULL,
            &mail_relay_system_shutdown,
            launch_mail_relay_subsystem,
            (void (*)(void))land_mail_relay_subsystem);
    }
}

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
    register_mail_relay_for_launch();

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

    const MailRelayConfig* config = app_config ? &app_config->mail_relay : NULL;
    if (!config || !config->Enabled) {
        log_this(SR_MAIL_RELAY, "Mail Relay disabled - skipping launch", LOG_LEVEL_STATE, 0);
        return 1;
    }

    mail_relay_system_shutdown = 0;

    log_this(SR_MAIL_RELAY, LOG_LINE_BREAK, LOG_LEVEL_STATE, 0);
    log_this(SR_MAIL_RELAY, "LAUNCH: " SR_MAIL_RELAY, LOG_LEVEL_STATE, 0);

    // Ensure the registry knows about Mail Relay before we start threads.
    register_mail_relay_for_launch();
    if (mailrelay_subsystem_id < 0) {
        log_this(SR_MAIL_RELAY, "LAUNCH: " SR_MAIL_RELAY " Failed: registry registration failed",
                 LOG_LEVEL_STATE, 0);
        update_subsystem_on_startup(SR_MAIL_RELAY, false);
        return 0;
    }

    // Initialize mail relay runtime and thread tracking
    if (!mailrelay_init()) {
        log_this(SR_MAIL_RELAY, "LAUNCH: " SR_MAIL_RELAY " Failed: runtime initialization failed",
                 LOG_LEVEL_STATE, 0);
        update_subsystem_on_startup(SR_MAIL_RELAY, false);
        return 0;
    }

    // Start worker threads for asynchronous queue processing.
    if (config->Workers > 0) {
        if (!mailrelay_workers_start(config->Workers)) {
            log_this(SR_MAIL_RELAY, "LAUNCH: " SR_MAIL_RELAY " Failed: worker start failed",
                     LOG_LEVEL_STATE, 0);
            update_subsystem_on_startup(SR_MAIL_RELAY, false);
            mailrelay_shutdown();
            return 0;
        }
    }

    // SendRawOnLaunch smoke test: enqueue one test message for workers to send.
    // This exercises the async path and is test-only.
    if (config->Test.SendRawOnLaunch && config->OutboundServerCount > 0) {
        log_this(SR_MAIL_RELAY,
                 "TEST: SendRawOnLaunch requested, enqueuing smoke test",
                 LOG_LEVEL_STATE, 0);

        MailRelayMessage msg = {0};
        mailrelay_message_init(&msg);
        if (config->Test.TestFrom && config->Test.TestFrom[0] != '\0') {
            mailrelay_message_set_from(&msg, config->Test.TestFrom);
        } else if (config->DefaultFrom && config->DefaultFrom[0] != '\0') {
            mailrelay_message_set_from(&msg, config->DefaultFrom);
        }
        if (config->Test.TestTo && config->Test.TestTo[0] != '\0') {
            mailrelay_message_add_to(&msg, config->Test.TestTo);
        }
        if (config->Test.TestSubject && config->Test.TestSubject[0] != '\0') {
            msg.subject = strdup(config->Test.TestSubject);
        }
        if (config->Test.TestBody && config->Test.TestBody[0] != '\0') {
            msg.text_body = strdup(config->Test.TestBody);
        }

        MailRelayStatus status = mailrelay_enqueue(&msg, 0);
        if (status == MAILRELAY_OK) {
            log_this(SR_MAIL_RELAY,
                     "TEST: SendRawOnLaunch smoke test enqueued",
                     LOG_LEVEL_STATE, 0);
        } else {
            log_this(SR_MAIL_RELAY,
                     "TEST: SendRawOnLaunch enqueue failed (status=%d)",
                     LOG_LEVEL_STATE, 1, (int)status);
        }

        mailrelay_message_free(&msg);
    }

    // Update registry and verify the subsystem reached RUNNING.
    update_subsystem_on_startup(SR_MAIL_RELAY, true);
    SubsystemState final_state = get_subsystem_state(mailrelay_subsystem_id);
    if (final_state != SUBSYSTEM_RUNNING) {
        log_this(SR_MAIL_RELAY,
                 "LAUNCH: " SR_MAIL_RELAY " Failed: unexpected registry state %s",
                 LOG_LEVEL_STATE, 1, subsystem_state_to_string(final_state));
        update_subsystem_on_startup(SR_MAIL_RELAY, false);
        mailrelay_shutdown();
        return 0;
    }

    log_this(SR_MAIL_RELAY, "LAUNCH: " SR_MAIL_RELAY " Success: running", LOG_LEVEL_STATE, 0);
    return 1;
}
