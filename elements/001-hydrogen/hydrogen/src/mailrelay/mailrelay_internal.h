/*
 * Mail Relay internal runtime state.
 *
 * This header is private to the mailrelay module. It exposes the runtime
 * structure and global pointer so that unit tests can inspect state without
 * relying on static globals. The public lifecycle API remains in
 * mailrelay/mailrelay.h.
 */

#ifndef MAILRELAY_INTERNAL_H
#define MAILRELAY_INTERNAL_H

// System includes
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>

// Project includes
#include <src/mailrelay/mailrelay_debounce.h>
#include <src/mailrelay/mailrelay_queue.h>
#include <src/mailrelay/mailrelay_repository.h>
#include <src/mailrelay/mailrelay_result.h>

/*
 * Per-event-key rate limit state. Fixed-window counter: resets when
 * MailRelay.Events.EventIntervalSeconds elapses.
 */
typedef struct MailRelayRateLimitEntry {
    char* event_key;
    time_t window_start;
    int count;
    struct MailRelayRateLimitEntry* next;
} MailRelayRateLimitEntry;

/*
 * Runtime state for the Mail Relay subsystem.
 *
 * A single instance is allocated by mailrelay_init() and freed by
 * mailrelay_shutdown(). All fields are protected by mutex once worker
 * threads exist; during Phase 3.1 the structure is primarily used for
 * lifecycle tracking and counters.
 */
typedef struct MailRelayRuntime {
    pthread_mutex_t mutex;        // Guards runtime fields and queue access
    pthread_cond_t cond;          // Worker sleep/wake signal
    bool initialized;             // True after successful mailrelay_init()
    bool shutdown_requested;      // Set true during shutdown to wake workers
    int worker_count;             // Number of active worker threads
    size_t queued_count;          // Lifetime queued message count
    size_t sending_count;         // Messages currently being sent
    size_t sent_count;            // Lifetime sent message count
    size_t failed_count;          // Lifetime failed message count
    size_t retry_count;           // Lifetime retry attempts
    size_t retrying_count;        // Messages scheduled for future retry
    size_t permanent_failures_count; // Messages that exhausted retry attempts
    time_t last_success_at;       // Timestamp of last successful send
    time_t last_failure_at;       // Timestamp of last failed/permanent send
    MailRelayResult last_error;   // Last recorded error (no secrets)
    MailRelayQueue* queue;        // In-memory queue (populated in Phase 3.2)
    MailRelayDebounceState* debounce; // Debounce/coalescing state (Phase 3.5)
    MailRelayRateLimitEntry* rate_limit_head; // Per-event-key rate limit state (Phase 6)
} MailRelayRuntime;

// Global runtime instance. Owned by mailrelay_init()/mailrelay_shutdown().
extern MailRelayRuntime* mailrelay_runtime;

/*
 * Check whether the runtime instance has been initialized.
 *
 * @return true if mailrelay_init() has succeeded and shutdown has not run
 */
bool mailrelay_runtime_is_initialized(void);

/*
 * Recover outbound mail queue rows stuck in 'sending' from a previous
 * instance. No-op unless Queue.Persist is enabled and a database target is
 * configured.
 */
bool mailrelay_recover_stale_sending_rows(void);

/*
 * Persist (if enabled) and enqueue a message onto a specific queue.
 *
 * Used by the public mailrelay_enqueue() path and by the debounce flush path
 * so that debounced/coalesced messages are also persisted when persistence is
 * enabled. Does not apply debounce logic.
 */
MailRelayStatus mailrelay_enqueue_to_queue(const MailRelayMessage* msg,
                                           int priority,
                                           MailRelayQueue* queue);

/*
 * Free all per-event-key rate limit entries.
 * Called from mailrelay_shutdown().
 */
void mailrelay_event_free_all_rate_limits(void);

/*
 * The following helpers are exposed (non-static) primarily so the Unity test
 * suite can exercise enqueue/persistence and recovery callbacks directly. They
 * are not part of the stable public API.
 */
void recovery_callback(MailRelayRepoResult* result, void* user_data);
void insert_callback(MailRelayRepoResult* result, void* user_data);
bool mailrelay_persist_message(const MailRelayMessage* msg,
                               int priority,
                               long long* out_queue_id);

#endif /* MAILRELAY_INTERNAL_H */
