/*
 * Mail Relay public internal API implementation.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_debounce.h>
#include <src/mailrelay/mailrelay_internal.h>
#include <src/mailrelay/mailrelay_repository.h>
#include <src/mailrelay/mailrelay_smtp.h>
#include <src/mailrelay/mailrelay_test_seams.h>
#include <src/mailrelay/mailrelay_workers.h>

#include <src/threads/threads.h>
#include <src/utils/utils_time.h>
#include <src/utils/utils_uuid.h>

// Global runtime instance. Owned by mailrelay_init()/mailrelay_shutdown().
MailRelayRuntime* mailrelay_runtime = NULL;

bool mailrelay_runtime_is_initialized(void) {
    return mailrelay_runtime != NULL && mailrelay_runtime->initialized;
}

static void recovery_callback(MailRelayRepoResult* result, void* user_data) {
    (void)user_data;
    if (!result) {
        return;
    }
    if (result->status == MAILRELAY_REPO_OK) {
        log_this(SR_MAIL_RELAY,
                 "Recovered %d stale sending mail queue row(s)",
                 LOG_LEVEL_STATE, 1, result->affected_rows);
    } else {
        log_this(SR_MAIL_RELAY,
                 "Mail Relay startup recovery failed: %s",
                 LOG_LEVEL_ERROR, 1,
                 result->error_message ? result->error_message : "unknown");
    }
}

bool mailrelay_recover_stale_sending_rows(void) {
    if (!app_config) {
        return false;
    }

    const MailRelayConfig* config = &app_config->mail_relay;
    if (!config->Queue.Persist) {
        return true;
    }
    if (!config->Database || config->Database[0] == '\0') {
        log_this(SR_MAIL_RELAY,
                 "Queue persistence enabled but no database target; skipping stale recovery",
                 LOG_LEVEL_ALERT, 0);
        return false;
    }

    int stale_seconds = config->Queue.StaleTimeoutSeconds;
    if (stale_seconds <= 0) {
        stale_seconds = 300;
    }

    time_t now = time(NULL);
    time_t stale_before = now - (time_t)stale_seconds;
    char iso_time[32];
    format_iso_time(stale_before, iso_time, sizeof(iso_time));

    MailRelayRepoQueueRecoverStale params = {
        .stale_before = iso_time
    };
    return mailrelay_repo_queue_recover_stale(&params, recovery_callback, NULL);
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
    runtime->sending_count = 0;
    runtime->sent_count = 0;
    runtime->failed_count = 0;
    runtime->retry_count = 0;
    runtime->retrying_count = 0;
    runtime->permanent_failures_count = 0;
    runtime->last_success_at = 0;
    runtime->last_failure_at = 0;
    runtime->queue = NULL;
    runtime->debounce = NULL;
    runtime->rate_limit_head = NULL;
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

    // Recover any mail queue rows left in 'sending' by a previous instance.
    mailrelay_recover_stale_sending_rows();

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
    mailrelay_event_free_all_rate_limits();

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

bool mailrelay_get_status(MailRelayStatusCounters* counters) {
    if (!counters) {
        return false;
    }

    memset(counters, 0, sizeof(MailRelayStatusCounters));
    if (app_config) {
        counters->enabled = app_config->mail_relay.Enabled;
    }

    if (!mailrelay_runtime_is_initialized() || !mailrelay_runtime) {
        return false;
    }

    pthread_mutex_lock(&mailrelay_runtime->mutex);
    counters->initialized = true;
    counters->queued = mailrelay_runtime->queued_count;
    counters->sending = mailrelay_runtime->sending_count;
    counters->sent = mailrelay_runtime->sent_count;
    counters->failed = mailrelay_runtime->failed_count;
    counters->retrying = mailrelay_runtime->retrying_count;
    counters->permanent_failures = mailrelay_runtime->permanent_failures_count;
    counters->last_success = mailrelay_runtime->last_success_at;
    counters->last_failure = mailrelay_runtime->last_failure_at;
    counters->worker_count = mailrelay_runtime->worker_count;
    if (mailrelay_runtime->queue) {
        counters->queue_depth = mailrelay_queue_size(mailrelay_runtime->queue);
    }
    pthread_mutex_unlock(&mailrelay_runtime->mutex);

    update_service_thread_metrics(&mailrelay_threads);
    counters->worker_count = mailrelay_threads.thread_count;

    return true;
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

/* Context used to capture the result of a queue insert callback. */
typedef struct {
    MailRelayRepoStatus status;
    long long queue_id;
} MailRelayInsertContext;

static void insert_callback(MailRelayRepoResult* result, void* user_data) {
    MailRelayInsertContext* ctx = (MailRelayInsertContext*)user_data;
    if (!result || !ctx) {
        return;
    }
    ctx->status = result->status;
    if (result->status == MAILRELAY_REPO_OK && result->data && json_is_object(result->data)) {
        json_t* qid = json_object_get(result->data, "queue_id");
        if (qid && json_is_integer(qid)) {
            ctx->queue_id = json_integer_value(qid);
        }
    }
}

static bool mailrelay_persist_message(const MailRelayMessage* msg,
                                      int priority,
                                      long long* out_queue_id) {
    if (!msg) {
        return false;
    }
    if (!app_config || !app_config->mail_relay.Queue.Persist) {
        return true;
    }
    if (!app_config->mail_relay.Database || app_config->mail_relay.Database[0] == '\0') {
        log_this(SR_MAIL_RELAY,
                 "Queue persistence enabled but no database target configured",
                 LOG_LEVEL_ERROR, 0);
        return false;
    }

    char uuid_str[UUID_STR_LEN];
    if (msg->message_id && msg->message_id[0] != '\0') {
        snprintf(uuid_str, sizeof(uuid_str), "%s", msg->message_id);
    } else {
        generate_uuid(uuid_str);
    }

    char* recipients_json = mailrelay_message_recipients_to_json(msg);
    if (!recipients_json) {
        log_this(SR_MAIL_RELAY, "Failed to serialize recipients for persistence", LOG_LEVEL_ERROR, 0);
        return false;
    }

    time_t now = time(NULL);
    char next_attempt_at[32];
    format_iso_time(now, next_attempt_at, sizeof(next_attempt_at));

    MailRelayInsertContext ctx = {
        .status = MAILRELAY_REPO_OK,
        .queue_id = 0
    };

    MailRelayRepoQueueInsert params = {
        .message_uuid = uuid_str,
        .priority = priority,
        .template_key = msg->template_key,
        .from_addr = msg->from,
        .reply_to = msg->reply_to,
        .recipients_json = recipients_json,
        .subject = msg->subject,
        .body_text = msg->text_body,
        .body_html = msg->html_body,
        .headers_json = msg->headers,
        .idempotency_key = msg->idempotency_key,
        .next_attempt_at = next_attempt_at
    };

    bool submitted = mailrelay_repo_queue_insert(&params, insert_callback, &ctx);
    free(recipients_json);

    if (!submitted || ctx.status != MAILRELAY_REPO_OK) {
        log_this(SR_MAIL_RELAY,
                 "Failed to persist mail queue row (status=%d)",
                 LOG_LEVEL_ERROR, 1, (int)ctx.status);
        return false;
    }

    if (out_queue_id) {
        *out_queue_id = ctx.queue_id;
    }
    return true;
}

MailRelayStatus mailrelay_enqueue_to_queue(const MailRelayMessage* msg,
                                           int priority,
                                           MailRelayQueue* queue) {
    if (!msg || !queue) {
        return MAILRELAY_INVALID_ARGS;
    }

    if (!mailrelay_persist_message(msg, priority, NULL)) {
        return MAILRELAY_PERSIST_FAILED;
    }

    MailRelayStatus status = mailrelay_queue_enqueue(queue, msg, priority);
    if (status == MAILRELAY_OK) {
        if (mailrelay_runtime) {
            pthread_mutex_lock(&mailrelay_runtime->mutex);
            mailrelay_runtime->queued_count++;
            pthread_mutex_unlock(&mailrelay_runtime->mutex);
        }
    }
    return status;
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

    return mailrelay_enqueue_to_queue(msg, priority, mailrelay_runtime->queue);
}
