/*
 * Mail Relay worker pool implementation.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_internal.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_queue.h>
#include <src/mailrelay/mailrelay_repository.h>
#include <src/mailrelay/mailrelay_result.h>
#include <src/mailrelay/mailrelay_retry.h>
#include <src/mailrelay/mailrelay_workers.h>

#include <src/threads/threads.h>
#include <src/utils/utils_time.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void worker_repo_noop_cb(MailRelayRepoResult* result, void* user_data) {
    (void)result;
    (void)user_data;
}

void worker_persist_outcome(const MailRelayQueueItem* item,
                            bool sent,
                            bool retrying,
                            const MailRelayResult* result) {
    if (!item || item->message.queue_id <= 0) {
        return;
    }
    if (!app_config || !app_config->mail_relay.Queue.Persist) {
        return;
    }

    long long queue_id = item->message.queue_id;
    int smtp_code = result ? result->smtp_code : 0;
    const char* smtp_text = (result && result->smtp_text[0] != '\0')
                            ? result->smtp_text : "";
    int server_index = 0;

    if (sent) {
        MailRelayRepoQueueMarkSent params = {
            .queue_id = queue_id,
            .smtp_code = smtp_code,
            .smtp_text = smtp_text,
            .server_index = server_index
        };
        (void)mailrelay_repo_queue_mark_sent(&params, worker_repo_noop_cb, NULL);
    } else if (retrying) {
        time_t next = time(NULL);
        if (item->next_attempt_at.tv_sec > 0) {
            next = item->next_attempt_at.tv_sec;
        }
        char next_iso[32];
        format_iso_time(next, next_iso, sizeof(next_iso));
        MailRelayRepoQueueReschedule params = {
            .queue_id = queue_id,
            .next_attempt_at = next_iso,
            .attempts = item->attempts + 1
        };
        (void)mailrelay_repo_queue_reschedule(&params, worker_repo_noop_cb, NULL);
    } else {
        MailRelayRepoQueueMarkFailed params = {
            .queue_id = queue_id,
            .smtp_code = smtp_code,
            .smtp_text = smtp_text,
            .server_index = server_index
        };
        (void)mailrelay_repo_queue_mark_failed(&params, worker_repo_noop_cb, NULL);
    }

    MailRelayRepoAttemptInsert attempt = {
        .queue_id = queue_id,
        .attempt_number = item->attempts + 1,
        .server_index = server_index,
        .success = sent,
        .smtp_code = smtp_code,
        .smtp_text = smtp_text,
        .duration_ms = result ? (long long)result->duration_ms : 0,
        .error_class = sent ? "success" : (retrying ? "transient" : "permanent")
    };
    (void)mailrelay_repo_attempt_insert(&attempt, worker_repo_noop_cb, NULL);
}

// Worker thread entry point.
void* mailrelay_worker_thread(void* arg) {
    (void)arg;

    add_service_thread(&mailrelay_threads, pthread_self());

    while (mailrelay_runtime_is_initialized() && !mailrelay_runtime->shutdown_requested) {
        MailRelayQueueItem item;
        MailRelayStatus status = mailrelay_queue_dequeue(mailrelay_runtime->queue, 1000, &item);

        if (status == MAILRELAY_TIMEOUT || status == MAILRELAY_SHUTDOWN) {
            // Shutdown with empty queue returns SHUTDOWN; continue so the
            // outer shutdown_requested check drives exit.
            if (status == MAILRELAY_SHUTDOWN) {
                break;
            }
            continue;
        }
        if (status != MAILRELAY_OK) {
            continue;
        }

        pthread_mutex_lock(&mailrelay_runtime->mutex);
        if (item.attempts > 0 && mailrelay_runtime->retrying_count > 0) {
            mailrelay_runtime->retrying_count--;
        }
        mailrelay_runtime->sending_count++;
        pthread_mutex_unlock(&mailrelay_runtime->mutex);

        const MailRelayConfig* config = app_config ? &app_config->mail_relay : NULL;
        if (!config || config->OutboundServerCount <= 0) {
            pthread_mutex_lock(&mailrelay_runtime->mutex);
            mailrelay_runtime->sending_count--;
            mailrelay_runtime->failed_count++;
            mailrelay_runtime->permanent_failures_count++;
            mailrelay_runtime->last_failure_at = time(NULL);
            pthread_mutex_unlock(&mailrelay_runtime->mutex);
            mailrelay_message_free(&item.message);
            continue;
        }

        const char* default_from = config->DefaultFrom;
        const char* app_name = (app_config && app_config->server.server_name)
                               ? app_config->server.server_name : "hydrogen";
        MailRelayResult result;
        mailrelay_result_init(&result);

        if (item.message.queue_id > 0 && config->Queue.Persist) {
            char claim[64];
            snprintf(claim, sizeof(claim), "w-%lu", (unsigned long)pthread_self());
            MailRelayRepoQueueMarkSending mark = {
                .queue_id = item.message.queue_id,
                .instance_id = app_name,
                .claim_token = claim
            };
            (void)mailrelay_repo_queue_mark_sending(&mark, worker_repo_noop_cb, NULL);
        }

        bool sent = mailrelay_send_raw(&item.message,
                                       &config->Servers[0],
                                       default_from,
                                       app_name,
                                       &result);

        if (sent) {
            pthread_mutex_lock(&mailrelay_runtime->mutex);
            mailrelay_runtime->sending_count--;
            mailrelay_runtime->sent_count++;
            mailrelay_runtime->last_success_at = time(NULL);
            pthread_mutex_unlock(&mailrelay_runtime->mutex);
            worker_persist_outcome(&item, true, false, &result);
        } else if (mailrelay_retry_should_retry(&result,
                                                 item.attempts,
                                                 config->Queue.RetryAttempts)) {
            int delay = mailrelay_retry_compute_delay(item.attempts,
                                                       config->Queue.InitialDelaySeconds,
                                                       config->Queue.MaxDelaySeconds);
            struct timespec next;
            clock_gettime(CLOCK_REALTIME, &next);
            next.tv_sec += delay;
            item.next_attempt_at = next;

            pthread_mutex_lock(&mailrelay_runtime->mutex);
            mailrelay_runtime->retry_count++;
            pthread_mutex_unlock(&mailrelay_runtime->mutex);

            MailRelayStatus enq_status = mailrelay_queue_enqueue_scheduled(
                mailrelay_runtime->queue,
                &item.message,
                item.priority,
                item.attempts + 1,
                &next);

            if (enq_status == MAILRELAY_OK) {
                pthread_mutex_lock(&mailrelay_runtime->mutex);
                mailrelay_runtime->sending_count--;
                mailrelay_runtime->retrying_count++;
                pthread_mutex_unlock(&mailrelay_runtime->mutex);
                worker_persist_outcome(&item, false, true, &result);
            } else {
                pthread_mutex_lock(&mailrelay_runtime->mutex);
                mailrelay_runtime->sending_count--;
                mailrelay_runtime->failed_count++;
                mailrelay_runtime->permanent_failures_count++;
                mailrelay_runtime->last_failure_at = time(NULL);
                mailrelay_runtime->last_error = result;
                pthread_mutex_unlock(&mailrelay_runtime->mutex);
                worker_persist_outcome(&item, false, false, &result);
                log_this(SR_MAIL_RELAY,
                         "Retry enqueue failed for message: status=%d",
                         LOG_LEVEL_ERROR, 1, enq_status);
            }
        } else {
            pthread_mutex_lock(&mailrelay_runtime->mutex);
            mailrelay_runtime->sending_count--;
            mailrelay_runtime->failed_count++;
            mailrelay_runtime->permanent_failures_count++;
            mailrelay_runtime->last_failure_at = time(NULL);
            mailrelay_runtime->last_error = result;
            pthread_mutex_unlock(&mailrelay_runtime->mutex);
            worker_persist_outcome(&item, false, false, &result);
        }

        mailrelay_message_free(&item.message);
    }

    remove_service_thread(&mailrelay_threads, pthread_self());
    return NULL;
}

bool mailrelay_workers_start(int count) {
    if (!mailrelay_runtime_is_initialized() || count <= 0) {
        return false;
    }

    pthread_mutex_lock(&mailrelay_runtime->mutex);
    mailrelay_runtime->worker_count = count;
    pthread_mutex_unlock(&mailrelay_runtime->mutex);

    for (int i = 0; i < count; i++) {
        pthread_t thread;
        if (pthread_create(&thread, NULL, mailrelay_worker_thread, NULL) != 0) {
            log_this(SR_MAIL_RELAY, "Failed to create mail relay worker thread", LOG_LEVEL_ERROR, 0);
            return false;
        }
        pthread_detach(thread);
    }

    log_this(SR_MAIL_RELAY, "Started %d mail relay worker thread(s)", LOG_LEVEL_DEBUG, 1, count);
    return true;
}

void mailrelay_workers_stop(void) {
    if (!mailrelay_runtime) {
        return;
    }

    pthread_mutex_lock(&mailrelay_runtime->mutex);
    mailrelay_runtime->shutdown_requested = true;
    pthread_mutex_unlock(&mailrelay_runtime->mutex);

    if (mailrelay_runtime->queue) {
        mailrelay_queue_shutdown(mailrelay_runtime->queue);
    }

    log_this(SR_MAIL_RELAY, "Worker stop requested", LOG_LEVEL_DEBUG, 0);
}
