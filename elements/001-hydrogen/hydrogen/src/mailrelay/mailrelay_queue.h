/*
 * Mail Relay bounded in-memory queue.
 *
 * Thread-safe FIFO-with-priority queue for MailRelayMessage items. Higher
 * priority values are dequeued before lower ones; within the same priority
 * the order is FIFO. The queue is bounded: enqueue returns MAILRELAY_QUEUE_FULL
 * when the configured capacity is reached.
 */

#ifndef MAILRELAY_QUEUE_H
#define MAILRELAY_QUEUE_H

// System includes
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
#include <time.h>

// Project includes
#include <src/mailrelay/mailrelay_message.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Default capacity when a configured value is unusable. */
#define MAILRELAY_QUEUE_DEFAULT_CAPACITY 1000

/*
 * Status codes returned by queue operations.
 */
typedef enum MailRelayStatus {
    MAILRELAY_OK = 0,
    MAILRELAY_INVALID_ARGS,
    MAILRELAY_QUEUE_FULL,
    MAILRELAY_SHUTDOWN,
    MAILRELAY_TIMEOUT,
    MAILRELAY_DISABLED,
} MailRelayStatus;

/*
 * A single queued item. The embedded message is deep-copied on enqueue so the
 * caller can free the original immediately. The item also carries scheduling
 * metadata used by workers, retry, and debounce logic.
 */
typedef struct MailRelayQueueItem {
    MailRelayMessage message;          // Deep-copied message
    int priority;                      // Higher values dequeued first
    struct timespec enqueued_at;       // Wall-clock enqueue time
    int attempts;                      // Number of send attempts so far
    struct timespec next_attempt_at;   // Next scheduled attempt
    struct MailRelayQueueItem* next;   // Next item in the linked list
} MailRelayQueueItem;

/*
 * Bounded in-memory queue.
 */
typedef struct MailRelayQueue {
    pthread_mutex_t mutex;             // Guards all fields
    pthread_cond_t not_empty;          // Signaled when items are available
    int capacity;                      // Maximum number of items
    int size;                          // Current number of items
    bool shutdown;                     // True after shutdown is requested
    MailRelayQueueItem* head;          // Highest-priority front item
    MailRelayQueueItem* tail;          // Tail pointer for FIFO insertion
} MailRelayQueue;

/*
 * Create a new bounded queue.
 *
 * @param capacity Maximum number of items (must be > 0).
 * @return A new queue, or NULL on failure.
 */
MailRelayQueue* mailrelay_queue_create(int capacity);

/*
 * Destroy a queue and free all remaining items.
 *
 * @param queue Queue to destroy.
 */
void mailrelay_queue_destroy(MailRelayQueue* queue);

/*
 * Enqueue a deep copy of a message.
 *
 * @param queue   Target queue.
 * @param message Message to copy into the queue.
 * @param priority Higher values are dequeued first.
 * @return MAILRELAY_OK on success, MAILRELAY_QUEUE_FULL if at capacity,
 *         MAILRELAY_INVALID_ARGS or MAILRELAY_SHUTDOWN otherwise.
 */
MailRelayStatus mailrelay_queue_enqueue(MailRelayQueue* queue,
                                        const MailRelayMessage* message,
                                        int priority);

/*
 * Enqueue a deep copy of a message with explicit retry scheduling metadata.
 *
 * @param queue          Target queue.
 * @param message        Message to copy into the queue.
 * @param priority       Higher values are dequeued first.
 * @param attempts       Number of attempts already made.
 * @param next_attempt_at Absolute time when this item becomes eligible for
 *                        sending. Items are only returned by dequeue once they
 *                        are due and the highest-priority due item.
 * @return MAILRELAY_OK on success, MAILRELAY_QUEUE_FULL if at capacity,
 *         MAILRELAY_INVALID_ARGS or MAILRELAY_SHUTDOWN otherwise.
 */
MailRelayStatus mailrelay_queue_enqueue_scheduled(MailRelayQueue* queue,
                                                  const MailRelayMessage* message,
                                                  int priority,
                                                  int attempts,
                                                  const struct timespec* next_attempt_at);

/*
 * Dequeue the next due item. Blocks until an item is due and available,
 * shutdown is signaled, or the timeout expires. Items with a future
 * next_attempt_at are skipped until their scheduled time arrives.
 *
 * @param queue      Source queue.
 * @param timeout_ms Maximum time to wait in milliseconds.
 * @param item       Out: copied item (caller must free message contents).
 * @return MAILRELAY_OK, MAILRELAY_SHUTDOWN, MAILRELAY_TIMEOUT, or
 *         MAILRELAY_INVALID_ARGS.
 */
MailRelayStatus mailrelay_queue_dequeue(MailRelayQueue* queue,
                                        int timeout_ms,
                                        MailRelayQueueItem* item);

/*
 * Request shutdown and wake any blocked waiters. After shutdown all enqueue
 * calls return MAILRELAY_SHUTDOWN and dequeue returns MAILRELAY_SHUTDOWN if
 * the queue is empty.
 *
 * @param queue Queue to shut down.
 */
void mailrelay_queue_shutdown(MailRelayQueue* queue);

/*
 * Current number of items in the queue.
 *
 * @param queue Queue to inspect.
 * @return Current size, or 0 for NULL queue.
 */
int mailrelay_queue_size(MailRelayQueue* queue);

/*
 * Configured capacity of the queue.
 *
 * @param queue Queue to inspect.
 * @return Capacity, or 0 for NULL queue.
 */
int mailrelay_queue_capacity(MailRelayQueue* queue);

#ifdef __cplusplus
}
#endif

#endif /* MAILRELAY_QUEUE_H */
