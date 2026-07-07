/*
 * Mail Relay debounce / coalescing implementation.
 */

#include <src/hydrogen.h>

#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_debounce.h>
#include <src/mailrelay/mailrelay_internal.h>
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_queue.h>
#include <src/mailrelay/mailrelay_test_seams.h>

#include <src/threads/threads.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Forward declarations (private helpers)
static void free_entry(MailRelayDebounceEntry* entry);
static MailRelayDebounceEntry* find_entry(MailRelayDebounceState* state,
                                          const char* key);
static MailRelayDebounceEntry* create_entry(const MailRelayMessage* msg,
                                            int priority,
                                            int debounce_seconds);
static bool replace_all(const char* src,
                        const char* placeholder,
                        const char* value,
                        char** out);
static bool build_coalesced_message(const MailRelayDebounceEntry* entry,
                                    MailRelayMessage* out);
static void enqueue_coalesced(MailRelayDebounceEntry* entry,
                              MailRelayQueue* queue);

MailRelayDebounceState* mailrelay_debounce_create(void) {
    MailRelayDebounceState* state = calloc(1, sizeof(MailRelayDebounceState));
    if (!state) {
        return NULL;
    }

    if (pthread_mutex_init(&state->mutex, NULL) != 0) {
        free(state);
        return NULL;
    }

    if (pthread_cond_init(&state->cond, NULL) != 0) {
        pthread_mutex_destroy(&state->mutex);
        free(state);
        return NULL;
    }

    state->shutdown_requested = false;
    state->thread_started = false;
    state->entries = NULL;
    state->count = 0;
    return state;
}

static void free_entry(MailRelayDebounceEntry* entry) {
    if (!entry) {
        return;
    }
    free(entry->key);
    mailrelay_message_free(&entry->message);
    free(entry);
}

void mailrelay_debounce_destroy(MailRelayDebounceState* state) {
    if (!state) {
        return;
    }

    MailRelayDebounceEntry* current = state->entries;
    while (current) {
        MailRelayDebounceEntry* next = current->next;
        free_entry(current);
        current = next;
    }
    state->entries = NULL;
    state->count = 0;

    pthread_cond_destroy(&state->cond);
    pthread_mutex_destroy(&state->mutex);
    free(state);
}

static MailRelayDebounceEntry* find_entry(MailRelayDebounceState* state,
                                          const char* key) {
    if (!state || !key) {
        return NULL;
    }
    for (MailRelayDebounceEntry* e = state->entries; e; e = e->next) {
        if (e->key && strcmp(e->key, key) == 0) {
            return e;
        }
    }
    return NULL;
}

static MailRelayDebounceEntry* create_entry(const MailRelayMessage* msg,
                                            int priority,
                                            int debounce_seconds) {
    MailRelayDebounceEntry* entry = calloc(1, sizeof(MailRelayDebounceEntry));
    if (!entry) {
        return NULL;
    }

    entry->key = strdup(msg->debounce_key);
    if (!entry->key) {
        free(entry);
        return NULL;
    }

    if (!mailrelay_message_copy(&entry->message, msg)) {
        free(entry->key);
        free(entry);
        return NULL;
    }

    entry->priority = priority;
    entry->count = 1;

    time_t now = mailrelay_seam_time ? mailrelay_seam_time() : time(NULL);
    entry->first_seen_at.tv_sec = now;
    entry->first_seen_at.tv_nsec = 0;
    entry->last_seen_at = entry->first_seen_at;
    entry->expires_at.tv_sec = now + (time_t)debounce_seconds;
    entry->expires_at.tv_nsec = 0;

    return entry;
}

bool mailrelay_debounce_submit(MailRelayDebounceState* state,
                               MailRelayQueue* queue,
                               const MailRelayMessage* msg,
                               int priority,
                               int debounce_seconds,
                               MailRelayStatus* out_status) {
    (void)queue;

    if (!state || !msg || !msg->debounce_key || msg->debounce_key[0] == '\0') {
        return false;
    }
    if (debounce_seconds <= 0) {
        return false;
    }

    pthread_mutex_lock(&state->mutex);

    // Lazy-start the expiry thread for the runtime debounce state. Tests may
    // pass a private state, in which case no thread is started here.
    bool needs_start = (mailrelay_runtime != NULL &&
                        state == mailrelay_runtime->debounce &&
                        !state->thread_started);
    pthread_mutex_unlock(&state->mutex);
    if (needs_start) {
        mailrelay_debounce_start();
    }

    pthread_mutex_lock(&state->mutex);

    MailRelayDebounceEntry* existing = find_entry(state, msg->debounce_key);
    if (existing) {
        existing->count++;
        time_t now = mailrelay_seam_time ? mailrelay_seam_time() : time(NULL);
        existing->last_seen_at.tv_sec = now;
        existing->last_seen_at.tv_nsec = 0;
        existing->expires_at.tv_sec = now + (time_t)debounce_seconds;
        existing->expires_at.tv_nsec = 0;
        pthread_cond_signal(&state->cond);
        pthread_mutex_unlock(&state->mutex);
        if (out_status) {
            *out_status = MAILRELAY_OK;
        }
        return true;
    }

    if (state->count >= MAILRELAY_DEBOUNCE_MAX_PENDING) {
        pthread_mutex_unlock(&state->mutex);
        if (out_status) {
            *out_status = MAILRELAY_QUEUE_FULL;
        }
        return true;
    }

    MailRelayDebounceEntry* entry = create_entry(msg, priority, debounce_seconds);
    if (!entry) {
        pthread_mutex_unlock(&state->mutex);
        if (out_status) {
            *out_status = MAILRELAY_QUEUE_FULL;
        }
        return true;
    }

    entry->next = state->entries;
    state->entries = entry;
    state->count++;
    pthread_cond_signal(&state->cond);
    pthread_mutex_unlock(&state->mutex);

    if (out_status) {
        *out_status = MAILRELAY_OK;
    }
    return true;
}

/*
 * Replace every occurrence of placeholder in src with value. The caller owns
 * *out. If placeholder is not found, *out is a copy of src. Returns false on
 * allocation failure or if src is NULL (in which case *out is set to NULL).
 */
static bool replace_all(const char* src,
                        const char* placeholder,
                        const char* value,
                        char** out) {
    if (!out) {
        return false;
    }
    *out = NULL;
    if (!src) {
        return true;
    }
    if (!placeholder || !value) {
        *out = strdup(src);
        return *out != NULL;
    }

    size_t placeholder_len = strlen(placeholder);
    size_t value_len = strlen(value);
    size_t count = 0;
    const char* tmp = src;
    while ((tmp = strstr(tmp, placeholder)) != NULL) {
        count++;
        tmp += placeholder_len;
    }

    if (count == 0) {
        *out = strdup(src);
        return *out != NULL;
    }

    size_t result_len = strlen(src) + count * (value_len - placeholder_len) + 1;
    char* result = malloc(result_len);
    if (!result) {
        return false;
    }

    char* dst = result;
    const char* cur = src;
    for (size_t i = 0; i < count; i++) {
        const char* match = strstr(cur, placeholder);
        size_t before_len = (size_t)(match - cur);
        memcpy(dst, cur, before_len);
        dst += before_len;
        memcpy(dst, value, value_len);
        dst += value_len;
        cur = match + placeholder_len;
    }
    strcpy(dst, cur);
    *out = result;
    return true;
}

static bool build_coalesced_message(const MailRelayDebounceEntry* entry,
                                    MailRelayMessage* out) {
    if (!entry || !out) {
        return false;
    }

    if (!mailrelay_message_copy(out, &entry->message)) {
        return false;
    }

    char count_str[32];
    snprintf(count_str, sizeof(count_str), "%d", entry->count);

    char summary[64];
    snprintf(summary, sizeof(summary), "%d event%s", entry->count,
             entry->count == 1 ? "" : "s");

    char* new_subject = NULL;
    char* new_text = NULL;
    char* new_html = NULL;

    if (!replace_all(out->subject, "%COUNT%", count_str, &new_subject)) {
        mailrelay_message_free(out);
        return false;
    }
    if (!replace_all(out->text_body, "%COUNT%", count_str, &new_text)) {
        free(new_subject);
        mailrelay_message_free(out);
        return false;
    }
    if (!replace_all(out->html_body, "%COUNT%", count_str, &new_html)) {
        free(new_subject);
        free(new_text);
        mailrelay_message_free(out);
        return false;
    }

    char* new_subject2 = NULL;
    char* new_text2 = NULL;
    char* new_html2 = NULL;

    if (!replace_all(new_subject ? new_subject : out->subject,
                     "%SUMMARY%", summary, &new_subject2)) {
        free(new_subject);
        free(new_text);
        free(new_html);
        mailrelay_message_free(out);
        return false;
    }
    if (!replace_all(new_text ? new_text : out->text_body,
                     "%SUMMARY%", summary, &new_text2)) {
        free(new_subject);
        free(new_text);
        free(new_html);
        free(new_subject2);
        mailrelay_message_free(out);
        return false;
    }
    if (!replace_all(new_html ? new_html : out->html_body,
                     "%SUMMARY%", summary, &new_html2)) {
        free(new_subject);
        free(new_text);
        free(new_html);
        free(new_subject2);
        free(new_text2);
        mailrelay_message_free(out);
        return false;
    }

    free(new_subject);
    free(new_text);
    free(new_html);

    if (new_subject2) {
        free(out->subject);
        out->subject = new_subject2;
    }
    if (new_text2) {
        free(out->text_body);
        out->text_body = new_text2;
    }
    if (new_html2) {
        free(out->html_body);
        out->html_body = new_html2;
    }

    return true;
}

static void enqueue_coalesced(MailRelayDebounceEntry* entry,
                              MailRelayQueue* queue) {
    if (!entry || !queue) {
        return;
    }

    MailRelayMessage coalesced;
    mailrelay_message_init(&coalesced);
    if (!build_coalesced_message(entry, &coalesced)) {
        log_this(SR_MAIL_RELAY,
                 "Failed to build coalesced debounce message for key '%s'",
                 LOG_LEVEL_ERROR, 1, entry->key ? entry->key : "(null)");
        return;
    }

    MailRelayStatus status = mailrelay_queue_enqueue(queue, &coalesced, entry->priority);
    if (status != MAILRELAY_OK) {
        log_this(SR_MAIL_RELAY,
                 "Failed to enqueue coalesced debounce message for key '%s': status=%d",
                 LOG_LEVEL_ERROR, 2, entry->key ? entry->key : "(null)", (int)status);
    }

    mailrelay_message_free(&coalesced);
}

int mailrelay_debounce_process_expired(MailRelayDebounceState* state,
                                       MailRelayQueue* queue,
                                       time_t now) {
    if (!state) {
        return -1;
    }

    pthread_mutex_lock(&state->mutex);

    MailRelayDebounceEntry* prev = NULL;
    MailRelayDebounceEntry* current = state->entries;
    time_t next_expiry = 0;
    bool have_next = false;

    while (current) {
        if (current->expires_at.tv_sec <= now) {
            MailRelayDebounceEntry* expired = current;
            if (prev) {
                prev->next = current->next;
            } else {
                state->entries = current->next;
            }
            current = current->next;
            state->count--;

            pthread_mutex_unlock(&state->mutex);
            enqueue_coalesced(expired, queue);
            free_entry(expired);
            pthread_mutex_lock(&state->mutex);
            continue;
        }

        if (!have_next || current->expires_at.tv_sec < next_expiry) {
            next_expiry = current->expires_at.tv_sec;
            have_next = true;
        }
        prev = current;
        current = current->next;
    }

    pthread_mutex_unlock(&state->mutex);

    if (!have_next) {
        return -1;
    }

    time_t diff = next_expiry - now;
    if (diff < 0) {
        diff = 0;
    }
    if (diff > INT_MAX / 1000) {
        diff = INT_MAX / 1000;
    }
    return (int)(diff * 1000);
}

void mailrelay_debounce_flush(MailRelayDebounceState* state, MailRelayQueue* queue) {
    if (!state || !queue) {
        return;
    }

    pthread_mutex_lock(&state->mutex);

    MailRelayDebounceEntry* current = state->entries;
    state->entries = NULL;
    state->count = 0;

    pthread_mutex_unlock(&state->mutex);

    while (current) {
        MailRelayDebounceEntry* next = current->next;
        enqueue_coalesced(current, queue);
        free_entry(current);
        current = next;
    }
}

// Debounce expiry thread entry point.
static void* mailrelay_debounce_thread(void* arg) {
    (void)arg;

    add_service_thread(&mailrelay_threads, pthread_self());

    while (mailrelay_runtime_is_initialized() && !mailrelay_runtime->shutdown_requested) {
        if (!mailrelay_runtime->debounce) {
            break;
        }

        MailRelayDebounceState* state = mailrelay_runtime->debounce;
        if (state->shutdown_requested) {
            break;
        }

        time_t now = mailrelay_seam_time ? mailrelay_seam_time() : time(NULL);
        int next_ms = mailrelay_debounce_process_expired(state,
                                                         mailrelay_runtime->queue,
                                                         now);

        pthread_mutex_lock(&state->mutex);
        if (mailrelay_runtime->shutdown_requested || state->shutdown_requested) {
            pthread_mutex_unlock(&state->mutex);
            break;
        }

        // Cap the cond wait at 1 second so a lost shutdown signal cannot
        // stall the thread forever. For pending entries with a nearer expiry,
        // use the smaller of next_ms and the cap.
        int wait_ms = 1000;
        if (next_ms >= 0 && next_ms < wait_ms) {
            wait_ms = next_ms;
        }

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += wait_ms / 1000;
        ts.tv_nsec += (wait_ms % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000L;
        }
        pthread_cond_timedwait(&state->cond, &state->mutex, &ts);
        pthread_mutex_unlock(&state->mutex);
    }

    remove_service_thread(&mailrelay_threads, pthread_self());
    return NULL;
}

bool mailrelay_debounce_start(void) {
    if (!mailrelay_runtime_is_initialized() || !mailrelay_runtime->debounce) {
        return false;
    }

    MailRelayDebounceState* state = mailrelay_runtime->debounce;

    pthread_mutex_lock(&state->mutex);
    if (state->thread_started) {
        pthread_mutex_unlock(&state->mutex);
        return true;
    }
    pthread_mutex_unlock(&state->mutex);

    pthread_t thread;
    if (pthread_create(&thread, NULL, mailrelay_debounce_thread, NULL) != 0) {
        log_this(SR_MAIL_RELAY, "Failed to create mail relay debounce thread",
                 LOG_LEVEL_ERROR, 0);
        return false;
    }
    pthread_detach(thread);

    pthread_mutex_lock(&state->mutex);
    state->thread_started = true;
    pthread_mutex_unlock(&state->mutex);

    log_this(SR_MAIL_RELAY, "Started mail relay debounce thread", LOG_LEVEL_DEBUG, 0);
    return true;
}

void mailrelay_debounce_stop(void) {
    if (!mailrelay_runtime || !mailrelay_runtime->debounce) {
        return;
    }

    MailRelayDebounceState* state = mailrelay_runtime->debounce;

    pthread_mutex_lock(&state->mutex);
    state->shutdown_requested = true;
    pthread_cond_signal(&state->cond);
    pthread_mutex_unlock(&state->mutex);

    log_this(SR_MAIL_RELAY, "Debounce stop requested", LOG_LEVEL_DEBUG, 0);
}
