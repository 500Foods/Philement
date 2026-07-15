/*
 * Mail Relay debounce / coalescing module.
 *
 * Collapses repeated event-generated mail with the same debounce_key into a
 * single queued message within a configured time window. The final message
 * replaces %COUNT% with the number of coalesced arrivals and %SUMMARY% with a
 * short pluralized summary (e.g., "5 events").
 */

#ifndef MAILRELAY_DEBOUNCE_H
#define MAILRELAY_DEBOUNCE_H

// System includes
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
#include <time.h>

// Project includes
#include <src/mailrelay/mailrelay_message.h>
#include <src/mailrelay/mailrelay_queue.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum number of pending debounce entries in memory. */
#define MAILRELAY_DEBOUNCE_MAX_PENDING 1024

/*
 * A pending debounce entry. Holds the first received message as a template,
 * the number of additional arrivals, and the expiry window.
 */
typedef struct MailRelayDebounceEntry {
    char* key;                         // Debounce grouping key (owned)
    MailRelayMessage message;          // First-arrived message (template)
    int priority;                      // Priority to use when enqueued
    int count;                         // Number of arrivals (including first)
    struct timespec first_seen_at;     // Wall-clock first arrival
    struct timespec last_seen_at;      // Wall-clock most recent arrival
    struct timespec expires_at;        // Wall-clock expiry
    struct MailRelayDebounceEntry* next;
} MailRelayDebounceEntry;

/*
 * In-memory debounce state. Protected by its own mutex; the expiry thread
 * waits on cond until the next expiry or a new submission.
 */
typedef struct MailRelayDebounceState {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool shutdown_requested;
    bool thread_started;
    MailRelayDebounceEntry* entries;
    int count;
} MailRelayDebounceState;

/*
 * Create a new debounce state.
 *
 * @return New state, or NULL on failure.
 */
MailRelayDebounceState* mailrelay_debounce_create(void);

/*
 * Destroy a debounce state and free all pending entries.
 *
 * @param state State to destroy.
 */
void mailrelay_debounce_destroy(MailRelayDebounceState* state);

/*
 * Start the debounce expiry thread. Uses the global mailrelay_runtime queue.
 *
 * @return true if the thread was started, false on failure.
 */
bool mailrelay_debounce_start(void);

/*
 * Signal the debounce expiry thread to stop.
 */
void mailrelay_debounce_stop(void);

/*
 * Submit a message for debounce processing.
 *
 * If the message has a non-empty debounce_key and debounce_seconds is > 0,
 * the message is deep-copied into pending state and false is returned. The
 * caller must NOT enqueue the message separately in that case.
 *
 * If debounce does not apply, false is returned and out_status (if non-NULL)
 * is left untouched. The caller should enqueue normally.
 *
 * @param state            Debounce state.
 * @param queue            Target queue for eventual expiry.
 * @param msg              Message to (possibly) debounce.
 * @param priority         Priority to use when the coalesced message is enqueued.
 * @param debounce_seconds Configured debounce window.
 * @param out_status       Optional status output.
 * @return true if the message was consumed by debounce, false otherwise.
 */
bool mailrelay_debounce_submit(MailRelayDebounceState* state,
                               MailRelayQueue* queue,
                               const MailRelayMessage* msg,
                               int priority,
                               int debounce_seconds,
                               MailRelayStatus* out_status);

/*
 * Process entries whose expiry time is <= now, enqueueing a coalesced message
 * for each. Returns milliseconds until the next pending expiry, or -1 if no
 * entries remain pending.
 *
 * @param state Debounce state.
 * @param queue Target queue for expired entries.
 * @param now   Current time to use for expiry checks.
 * @return Milliseconds until next check, or -1 if none pending.
 */
int mailrelay_debounce_process_expired(MailRelayDebounceState* state,
                                       MailRelayQueue* queue,
                                       time_t now);

/*
 * Flush all pending entries into the queue immediately, regardless of expiry.
 * Used during shutdown so no pending mail is lost.
 *
 * @param state Debounce state.
 * @param queue Target queue.
 */
void mailrelay_debounce_flush(MailRelayDebounceState* state, MailRelayQueue* queue);

/*
 * The following helpers are exposed (non-static) primarily so the Unity test
 * suite can exercise their NULL-guard and edge-case branches directly. They are
 * not part of the stable public API.
 */
void mailrelay_debounce_free_entry(MailRelayDebounceEntry* entry);
MailRelayDebounceEntry* mailrelay_debounce_find_entry(MailRelayDebounceState* state,
                                                     const char* key);
bool mailrelay_debounce_replace_all(const char* src,
                                   const char* placeholder,
                                   const char* value,
                                   char** out);
bool mailrelay_debounce_build_coalesced_message(const MailRelayDebounceEntry* entry,
                                               MailRelayMessage* out);
void mailrelay_debounce_enqueue_coalesced(MailRelayDebounceEntry* entry,
                                          MailRelayQueue* queue);

/* Internal helpers exposed (non-static) for the Unity test suite. Not part of
 * the stable public API. */
MailRelayDebounceEntry* create_entry(const MailRelayMessage* msg,
                                     int priority,
                                     int debounce_seconds);
void* mailrelay_debounce_thread(void* arg);

#ifdef __cplusplus
}
#endif

#endif /* MAILRELAY_DEBOUNCE_H */
