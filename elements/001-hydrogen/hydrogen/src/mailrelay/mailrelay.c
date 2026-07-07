/*
 * Mail Relay public internal API implementation.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_debounce.h>
#include <src/mailrelay/mailrelay_internal.h>
#include <src/mailrelay/mailrelay_smtp.h>
#include <src/mailrelay/mailrelay_test_seams.h>
#include <src/mailrelay/mailrelay_workers.h>

#include <src/threads/threads.h>

// Global runtime instance. Owned by mailrelay_init()/mailrelay_shutdown().
MailRelayRuntime* mailrelay_runtime = NULL;

bool mailrelay_runtime_is_initialized(void) {
    return mailrelay_runtime != NULL && mailrelay_runtime->initialized;
}

bool mailrelay_init(void) {
    mailrelay_reset_seams();

    if (mailrelay_runtime_is_initialized()) {
        return true;
    }

    MailRelayRuntime* runtime = calloc(1, sizeof(MailRelayRuntime));
    if (!runtime) {
        log_this(SR_MAIL_RELAY, "Failed to allocate mail relay runtime", LOG_LEVEL_ERROR, 0);
        return false;
    }

    if (pthread_mutex_init(&runtime->mutex, NULL) != 0) {
        free(runtime);
        log_this(SR_MAIL_RELAY, "Failed to initialize mail relay runtime mutex", LOG_LEVEL_ERROR, 0);
        return false;
    }

    if (pthread_cond_init(&runtime->cond, NULL) != 0) {
        pthread_mutex_destroy(&runtime->mutex);
        free(runtime);
        log_this(SR_MAIL_RELAY, "Failed to initialize mail relay runtime condition", LOG_LEVEL_ERROR, 0);
        return false;
    }

    runtime->initialized = true;
    runtime->shutdown_requested = false;
    runtime->worker_count = 0;
    runtime->queued_count = 0;
    runtime->sent_count = 0;
    runtime->failed_count = 0;
    runtime->retry_count = 0;
    runtime->queue = NULL;
    mailrelay_result_init(&runtime->last_error);

    int capacity = MAILRELAY_QUEUE_DEFAULT_CAPACITY;
    if (app_config && app_config->mail_relay.Queue.MaxInMemory > 0) {
        capacity = app_config->mail_relay.Queue.MaxInMemory;
    }
    runtime->queue = mailrelay_queue_create(capacity);
    if (!runtime->queue) {
        pthread_cond_destroy(&runtime->cond);
        pthread_mutex_destroy(&runtime->mutex);
        free(runtime);
        log_this(SR_MAIL_RELAY, "Failed to create mail relay queue", LOG_LEVEL_ERROR, 0);
        return false;
    }

    runtime->debounce = mailrelay_debounce_create();
    if (!runtime->debounce) {
        mailrelay_queue_destroy(runtime->queue);
        pthread_cond_destroy(&runtime->cond);
        pthread_mutex_destroy(&runtime->mutex);
        free(runtime);
        log_this(SR_MAIL_RELAY, "Failed to create mail relay debounce state", LOG_LEVEL_ERROR, 0);
        return false;
    }

    mailrelay_runtime = runtime;

    init_service_threads(&mailrelay_threads, SR_MAIL_RELAY);

    log_this(SR_MAIL_RELAY, "Runtime initialized", LOG_LEVEL_DEBUG, 0);
    return true;
}

void mailrelay_shutdown(void) {
    if (!mailrelay_runtime) {
        return;
    }

    mail_relay_system_shutdown = 1;
    mailrelay_workers_stop();
    mailrelay_debounce_stop();

    // Drain tracked worker threads with a bounded wait.
    bool drained = false;
    for (int i = 0; i < 100; i++) {
        update_service_thread_metrics(&mailrelay_threads);
        if (mailrelay_threads.thread_count == 0) {
            drained = true;
            break;
        }
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 100000000; // 100 milliseconds
        nanosleep(&ts, NULL);
    }

    if (!drained) {
        log_this(SR_MAIL_RELAY,
                 "Shutdown: %d worker thread(s) still active after drain timeout",
                 LOG_LEVEL_ALERT, 1, mailrelay_threads.thread_count);
    }

    if (mailrelay_runtime->debounce && mailrelay_runtime->queue) {
        mailrelay_debounce_flush(mailrelay_runtime->debounce, mailrelay_runtime->queue);
    }

    pthread_mutex_destroy(&mailrelay_runtime->mutex);
    pthread_cond_destroy(&mailrelay_runtime->cond);

    if (mailrelay_runtime->debounce) {
        mailrelay_debounce_destroy(mailrelay_runtime->debounce);
        mailrelay_runtime->debounce = NULL;
    }

    if (mailrelay_runtime->queue) {
        mailrelay_queue_destroy(mailrelay_runtime->queue);
        mailrelay_runtime->queue = NULL;
    }

    free(mailrelay_runtime);
    mailrelay_runtime = NULL;

    // Reset thread tracking so the next launch starts clean.
    init_service_threads(&mailrelay_threads, SR_MAIL_RELAY);

    log_this(SR_MAIL_RELAY, "Runtime shutdown complete", LOG_LEVEL_DEBUG, 0);
}

bool mailrelay_send_raw(const MailRelayMessage* msg,
                        const OutboundServer* server,
                        const char* default_from,
                        const char* app_name,
                        MailRelayResult* out) {
    if (!msg || !server || !out) {
        if (out) {
            mailrelay_result_init(out);
            snprintf(out->error, sizeof(out->error), "MAIL_RAW_INVALID_ARGS");
        }
        return false;
    }
    return mailrelay_smtp_send(msg, server, default_from, app_name, out);
}

MailRelayStatus mailrelay_enqueue(const MailRelayMessage* msg, int priority) {
    if (!msg) {
        return MAILRELAY_INVALID_ARGS;
    }
    if (!mailrelay_runtime_is_initialized() || mailrelay_runtime->shutdown_requested) {
        return MAILRELAY_SHUTDOWN;
    }
    if (app_config && !app_config->mail_relay.Enabled) {
        return MAILRELAY_DISABLED;
    }

    char err[256];
    if (!mailrelay_validate_message(msg, err, sizeof(err))) {
        return MAILRELAY_INVALID_ARGS;
    }

    int debounce_seconds = 0;
    if (app_config && app_config->mail_relay.Queue.DebounceSeconds > 0) {
        debounce_seconds = app_config->mail_relay.Queue.DebounceSeconds;
    }

    if (msg->debounce_key && msg->debounce_key[0] != '\0' && debounce_seconds > 0) {
        MailRelayStatus debounce_status = MAILRELAY_OK;
        bool consumed = mailrelay_debounce_submit(mailrelay_runtime->debounce,
                                                  mailrelay_runtime->queue,
                                                  msg,
                                                  priority,
                                                  debounce_seconds,
                                                  &debounce_status);
        if (consumed) {
            if (debounce_status == MAILRELAY_OK) {
                pthread_mutex_lock(&mailrelay_runtime->mutex);
                mailrelay_runtime->queued_count++;
                pthread_mutex_unlock(&mailrelay_runtime->mutex);
            }
            return debounce_status;
        }
    }

    MailRelayStatus status = mailrelay_queue_enqueue(mailrelay_runtime->queue, msg, priority);
    if (status == MAILRELAY_OK) {
        pthread_mutex_lock(&mailrelay_runtime->mutex);
        mailrelay_runtime->queued_count++;
        pthread_mutex_unlock(&mailrelay_runtime->mutex);
    }
    return status;
}
