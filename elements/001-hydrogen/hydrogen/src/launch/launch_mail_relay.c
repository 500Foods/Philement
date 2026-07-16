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

// System headers
#include <pthread.h>
#include <string.h>
#include <time.h>

// Local includes
#include "launch.h"
#include "launch_mail_relay.h"

// Registry integration
#include <src/registry/registry_integration.h>

// Mail relay includes
#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_otp.h>
#include <src/mailrelay/mailrelay_smtp.h>
#include <src/mailrelay/mailrelay_workers.h>
#include <src/mailrelay/mailrelay_repository.h>

// Database includes (to wait for the query cache to be populated before the
// synchronous OTP launch seam runs).
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database_cache.h>

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
 void register_mail_relay_for_launch(void) {
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

/*
 * Launch-time failing transport seam (gated by Test.FailNextSendOnLaunch).
 * Fails its first g_launch_fail_remaining attempts with a retryable error,
 * then succeeds. Mirrors the unit-test mock_transport contract. On the first
 * success it restores the default (real) transport so runtime sending is
 * unaffected. Only installed in the launch seam; inert otherwise.
 */
static pthread_mutex_t g_launch_transport_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_launch_fail_remaining = 0;

 bool launch_fail_transport(const MailRelaySmtpRequest* req, MailRelayResult* out) {
    (void)req;
    bool fail = false;
    pthread_mutex_lock(&g_launch_transport_mutex);
    if (g_launch_fail_remaining > 0) {
        g_launch_fail_remaining--;
        fail = true;
    }
    pthread_mutex_unlock(&g_launch_transport_mutex);

    mailrelay_result_init(out);
    if (fail) {
        out->success = false;
        out->retryable = true;
        out->smtp_code = 421;
        snprintf(out->error, sizeof(out->error), "MAILRELAY_LAUNCH_INJECTED_FAILURE");
        log_this(SR_MAIL_RELAY,
                 "MAILRELAY_LAUNCH_SEND_RETRY: injected transient SMTP failure (remaining=%d)",
                 LOG_LEVEL_STATE, 1, g_launch_fail_remaining);
        return false;
    }

    // Final attempt: perform the genuine send so the message is actually
    // delivered (and the retry path ends with a real success).
    bool ok = mailrelay_smtp_transport_real(req, out);
    log_this(SR_MAIL_RELAY, "MAILRELAY_LAUNCH_SEND_OK", LOG_LEVEL_STATE, 0);
    mailrelay_smtp_reset_transport();
    return ok;
}

/* Recipient/purpose used by the OTP launch seam (Seam A). */
#define MAILRELAY_OTP_LAUNCH_RECIPIENT "mailrelay-otp-launch@hydrogen.local"
#define MAILRELAY_OTP_LAUNCH_CODE      "123456"

/*
 * Wait until the Mail Relay database's query cache contains the OTP insert
 * QueryRef (112). The OTP launch seam issues synchronous OTP queries that
 * require cached QueryRefs, but the database bootstrap that populates the cache
 * runs asynchronously and may still be in flight when this subsystem launches
 * (the cache object itself may not even exist yet). Block briefly (polling)
 * until the cache exists and holds the OTP insert QueryRef, or time out.
 *
 * Returns true once the cache holds the OTP insert QueryRef.
 */
 bool launch_wait_for_mailrelay_otp_cache(unsigned int timeout_ms) {
    if (!app_config) {
        return false;
    }

    const char* db = app_config->mail_relay.Database;
    if (db == NULL || db[0] == '\0') {
        if (app_config->databases.connection_count == 1 &&
            app_config->databases.connections[0].name &&
            app_config->databases.connections[0].name[0] != '\0') {
            db = app_config->databases.connections[0].name;
        }
    }
    if (db == NULL || db[0] == '\0') {
        return false;
    }

    unsigned int waited = 0;
    const unsigned int step_ms = 25;
    while (waited < timeout_ms) {
        DatabaseQueue* q = database_queue_manager_get_database(global_queue_manager, db);
        if (q != NULL && q->query_cache != NULL &&
            query_cache_lookup(q->query_cache, MAILRELAY_QREF_OTP_INSERT,
                               SR_MAIL_RELAY) != NULL) {
            return true;
        }
        struct timespec ts = {0, (long)step_ms * 1000000L};
        nanosleep(&ts, NULL);
        waited += step_ms;
    }
    return false;
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

    // Seam B — forced transient SMTP failure (drives Test 57 retry coverage).
    // Install a launch-scoped transport that fails once with a retryable error
    // and then succeeds, so the worker retry path in mailrelay_workers.c /
    // mailrelay_retry.c runs during startup. Only active when explicitly set.
    if (config->Test.FailNextSendOnLaunch && config->Workers > 0
        && config->OutboundServerCount > 0) {
        pthread_mutex_lock(&g_launch_transport_mutex);
        g_launch_fail_remaining = 1;
        pthread_mutex_unlock(&g_launch_transport_mutex);
        mailrelay_smtp_set_transport(launch_fail_transport);
        log_this(SR_MAIL_RELAY,
                 "TEST: FailNextSendOnLaunch requested, installing transient-failure transport",
                 LOG_LEVEL_STATE, 0);
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

    // Seam A — OTP send + self-verify (drives Test 58 coverage).
    // Generate a fixed, known OTP, send it through the real templated worker
    // path, then immediately verify it against the same recipient/purpose.
    // Test-only: inert unless Test.SendOtpOnLaunch is set. Restores the default
    // transport first so this send is never caught by Seam B's failing transport.
    if (config->Test.SendOtpOnLaunch) {
        mailrelay_smtp_reset_transport();

        // The synchronous OTP queries below require the database query cache to
        // be populated. That cache is filled by the asynchronous database
        // bootstrap, which can still be in flight when this subsystem launches,
        // so wait for it before issuing the OTP queries.
        if (!launch_wait_for_mailrelay_otp_cache(15000)) {
            log_this(SR_MAIL_RELAY,
                     "TEST: SendOtpOnLaunch skipped - Mail Relay database query cache not ready",
                     LOG_LEVEL_STATE, 0);
        } else {
        mailrelay_otp_set_fixed_code(MAILRELAY_OTP_LAUNCH_CODE);

        MailRelayOtpSendRequest send_req;
        memset(&send_req, 0, sizeof(send_req));
        send_req.email = MAILRELAY_OTP_LAUNCH_RECIPIENT;
        send_req.account_id = 0;
        send_req.purpose_a66 = MAIL_OTP_PURPOSE_EMAIL_VERIFY;
        send_req.digits = (int)strlen(MAILRELAY_OTP_LAUNCH_CODE);
        send_req.expiry_seconds = 300;
        send_req.max_attempts = 5;
        send_req.priority = 0;
        send_req.app_name = "Hydrogen";

        MailRelayOtpSendResponse send_resp;
        mailrelay_otp_send_response_init(&send_resp);
        char otp_err[256];
        otp_err[0] = '\0';
        MailRelayStatus otp_status =
            mailrelay_otp_generate_and_send(&send_req, &send_resp, otp_err, sizeof(otp_err));

        if (otp_status == MAILRELAY_OK) {
            log_this(SR_MAIL_RELAY,
                     "MAILRELAY_OTP_LAUNCH_SENT recipient=" MAILRELAY_OTP_LAUNCH_RECIPIENT,
                     LOG_LEVEL_STATE, 0);
        } else {
            log_this(SR_MAIL_RELAY,
                     "MAILRELAY_OTP_LAUNCH_SEND_FAILED status=%d %s",
                     LOG_LEVEL_STATE, 2, (int)otp_status, otp_err);
        }

        MailRelayOtpVerifyRequest verify_req;
        memset(&verify_req, 0, sizeof(verify_req));
        verify_req.email = MAILRELAY_OTP_LAUNCH_RECIPIENT;
        verify_req.purpose_a66 = MAIL_OTP_PURPOSE_EMAIL_VERIFY;
        verify_req.code = MAILRELAY_OTP_LAUNCH_CODE;

        MailRelayOtpVerifyResponse verify_resp;
        mailrelay_otp_verify_response_init(&verify_resp);
        char verify_err[256];
        verify_err[0] = '\0';
        MailRelayStatus verify_status =
            mailrelay_otp_verify(&verify_req, &verify_resp, verify_err, sizeof(verify_err));

        if (verify_status == MAILRELAY_OK) {
            log_this(SR_MAIL_RELAY,
                     "MAILRELAY_OTP_LAUNCH_VERIFIED recipient=" MAILRELAY_OTP_LAUNCH_RECIPIENT,
                     LOG_LEVEL_STATE, 0);
        } else {
            log_this(SR_MAIL_RELAY,
                     "MAILRELAY_OTP_LAUNCH_VERIFY_FAILED status=%d %s",
                     LOG_LEVEL_STATE, 2, (int)verify_status, verify_err);
        }

        mailrelay_otp_clear_fixed_code();
        mailrelay_otp_send_response_free(&send_resp);
        }
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
