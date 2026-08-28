/*
 * Hydrogen Database Pending Results
 * Synchronous query execution with timeout support
 */

#ifndef DATABASE_PENDING_H
#define DATABASE_PENDING_H

#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#include "database_types.h"

// Forward declarations
typedef struct QueryResult QueryResult;

/**
 * @brief Pending query result structure for synchronous execution
 *
 * Manages the state of a query that is waiting for completion.
 * Uses condition variables for thread synchronization.
 */
typedef struct PendingQueryResult {
    char* query_id;                    /**< Unique identifier for this query */
    QueryResult* result;               /**< Result data (NULL until complete) */
    bool completed;                    /**< Completion flag */
    bool timed_out;                    /**< Timeout flag */
    pthread_mutex_t result_lock;       /**< Protects result access */
    pthread_cond_t result_ready;       /**< Signals completion */
    time_t submitted_at;               /**< Submission timestamp */
    int timeout_seconds;               /**< Query-specific timeout */
    size_t array_index;                /**< Current slot in manager->results[] (enables O(1) swap-remove) */
    struct PendingQueryResult* hash_next; /**< Intrusive chain link for the manager's query_id hash index */
} PendingQueryResult;

/**
 * @brief Manager for all pending query results
 *
 * Thread-safe container for tracking all active pending results.
 *
 * Lookups by query_id (register/signal/find/cancel) are served by an
 * intrusive chained hash index (hash_buckets) keyed on query_id, so they
 * are O(1) average instead of scanning the results[] array. The array
 * itself is kept (unordered) for cheap enumeration in cleanup_expired();
 * removals use swap-with-last (O(1)) rather than shifting the tail.
 */
typedef struct PendingResultManager {
    PendingQueryResult** results;      /**< Unordered array of pending results */
    size_t count;                      /**< Current number of results */
    size_t capacity;                   /**< Allocated capacity of results[] */
    pthread_mutex_t manager_lock;      /**< Protects results[] and hash_buckets[] */
    PendingQueryResult** hash_buckets; /**< Chained hash index buckets, keyed by query_id */
    size_t hash_bucket_count;          /**< Number of buckets in hash_buckets[] */
} PendingResultManager;

/**
 * @brief Initialize the pending result manager
 *
 * @return Pointer to initialized manager, or NULL on failure
 */
PendingResultManager* pending_result_manager_create(const char* dqm_label);

/**
 * @brief Destroy the pending result manager and all pending results
 *
 * @param manager Manager to destroy
 */
void pending_result_manager_destroy(PendingResultManager* manager, const char* dqm_label);

/**
 * @brief Register a new pending result
 *
 * Creates and registers a pending result with the given query ID and timeout.
 *
 * @param manager The pending result manager
 * @param query_id Unique identifier for the query
 * @param timeout_seconds Timeout in seconds
 * @return Pointer to the pending result, or NULL on failure
 */
PendingQueryResult* pending_result_register(
    PendingResultManager* manager,
    const char* query_id,
    int timeout_seconds,
    const char* dqm_label
);

/**
 * @brief Wait for a pending result to complete
 *
 * Blocks the calling thread until the result is ready or timeout occurs.
 *
 * @param pending The pending result to wait for
 * @return 0 on success, -1 on timeout or error
 */
int pending_result_wait(PendingQueryResult* pending, const char* dqm_label);

/**
 * @brief Signal that a query result is ready
 *
 * Called by the DQM worker thread when a query completes.
 *
 * @param manager The pending result manager
 * @param query_id The query ID that completed
 * @param result The query result (ownership transferred)
 * @return true if result was found and signaled, false otherwise
 */
bool pending_result_signal_ready(
    PendingResultManager* manager,
    const char* query_id,
    QueryResult* result,
    const char* dqm_label
);

/**
 * @brief Get the result from a completed pending query
 *
 * Should only be called after pending_result_wait() returns successfully.
 *
 * @param pending The pending result
 * @return Pointer to the result (ownership remains with pending result)
 */
QueryResult* pending_result_get(const PendingQueryResult* pending);

/**
 * @brief Look up a pending result by query_id (does not remove it)
 *
 * The returned pointer remains owned by the manager. Callers must not free
 * it and should treat it as unstable across other manager mutations unless
 * they hold manager_lock (status/age helpers copy fields under lock).
 *
 * @param manager The pending result manager
 * @param query_id Query identifier to find
 * @return Pending entry, or NULL if not found
 */
PendingQueryResult* pending_result_find(PendingResultManager* manager, const char* query_id);

/**
 * @brief Best-effort cancel: mark a pending query timed out and wake waiters
 *
 * Does not interrupt an in-flight engine execute; that path is owned by the
 * connection watchdog cancel hooks. This only abandons the waiter side.
 *
 * @return true if a matching pending entry was found and signaled
 */
bool pending_result_cancel(PendingResultManager* manager, const char* query_id, const char* dqm_label);

/**
 * @brief Check if a pending result has completed
 *
 * @param pending The pending result
 * @return true if completed, false otherwise
 */
bool pending_result_is_completed(const PendingQueryResult* pending);

/**
 * @brief Check if a pending result has timed out
 *
 * @param pending The pending result
 * @return true if timed out, false otherwise
 */
bool pending_result_is_timed_out(const PendingQueryResult* pending);

/**
 * @brief Clean up expired pending results
 *
 * Removes results that have been waiting longer than their timeout period.
 *
 * @param manager The pending result manager
 * @return Number of results cleaned up
 */
size_t pending_result_cleanup_expired(PendingResultManager* manager, const char* dqm_label);

/**
 * @brief Unregister and clean up a completed pending result
 *
 * Removes a pending result from the manager after the caller has consumed the result.
 * Frees the pending result structure, its query_id, the contained QueryResult
 * (using database_engine_cleanup_result), and destroys synchronization primitives.
 * Must be called after pending_result_wait() returns and the result has been read.
 *
 * @param manager The pending result manager
 * @param pending The pending result to remove and free
 * @param dqm_label Label for logging purposes
 */
void pending_result_unregister(PendingResultManager* manager, PendingQueryResult* pending, const char* dqm_label);

/**
 * @brief Get the global pending result manager instance
 *
 * @return Pointer to the global manager
 */
PendingResultManager* get_pending_result_manager(void);

/**
 * @brief Cleanup the global pending result manager
 * Should be called during database subsystem shutdown
 *
 * @param dqm_label Label for logging purposes
 */
void cleanup_global_pending_manager(const char* dqm_label);

/**
 * @brief Wait for multiple pending results to complete
 *
 * Waits for all pending results in the array to complete or timeout.
 * Returns when all queries have finished or the collective timeout is reached.
 *
 * @param pendings Array of pending results to wait for
 * @param count Number of pending results in the array
 * @param collective_timeout_seconds Maximum time to wait for all queries
 * @param dqm_label Label for logging purposes
 * @return 0 on success (all completed), -1 on timeout or error
 */
int pending_result_wait_multiple(PendingQueryResult **pendings, size_t count,
                                int collective_timeout_seconds, const char* dqm_label);

/**
 * @brief Compute the bucket index for a query_id in a hash index of a given size
 *
 * @param query_id Query identifier to hash (NULL-safe: returns 0)
 * @param bucket_count Number of buckets (0-safe: returns 0)
 * @return Bucket index in [0, bucket_count)
 */
size_t pending_hash_index(const char* query_id, size_t bucket_count);

/**
 * @brief Look up a pending result by query_id via the manager's hash index (O(1) average)
 *
 * @param manager The pending result manager
 * @param query_id Query identifier to find
 * @return Matching pending entry, or NULL if not present
 */
PendingQueryResult* pending_hash_find(const PendingResultManager* manager, const char* query_id);

/**
 * @brief Insert a pending result into the manager's hash index
 *
 * @param manager The pending result manager
 * @param pending Entry to insert (its hash_next is overwritten)
 */
void pending_hash_insert(PendingResultManager* manager, PendingQueryResult* pending);

/**
 * @brief Remove a pending result from the manager's hash index by identity
 *
 * @param manager The pending result manager
 * @param pending Entry to unlink (matched by pointer identity, not just query_id)
 */
void pending_hash_remove(PendingResultManager* manager, PendingQueryResult* pending);

/**
 * @brief Rebuild the hash index with a new bucket count
 *
 * Rehashes every entry currently in manager->results[0..count-1]. Called
 * whenever results[] capacity grows so the average chain length stays
 * short. Failure to resize is non-fatal: the existing (smaller) index
 * remains valid, just with longer chains.
 *
 * @param manager The pending result manager
 * @param new_bucket_count Desired number of buckets
 * @return true on success, false if the new bucket array could not be allocated
 */
bool pending_hash_resize(PendingResultManager* manager, size_t new_bucket_count);

/**
 * @brief Remove one entry from results[]/hash_buckets[] via swap-with-last (O(1))
 *
 * Caller must hold manager->manager_lock. Does not touch pending's
 * synchronization primitives or free anything; it only detaches the entry
 * from the manager's bookkeeping structures.
 *
 * @param manager The pending result manager
 * @param pending Entry to detach (must currently be registered in manager)
 */
void pending_result_detach_locked(PendingResultManager* manager, PendingQueryResult* pending);

/**
 * @brief Clean up expired pending results; caller must already hold manager_lock
 *
 * Shared implementation used both by pending_result_cleanup_expired() (which
 * acquires the lock itself) and by pending_result_register() (which is
 * already holding the lock and wants to reclaim expired slots before
 * growing results[]).
 *
 * @param manager The pending result manager
 * @param dqm_label Label for logging purposes
 * @return Number of results cleaned up
 */
size_t pending_result_reap_expired_locked(PendingResultManager* manager, const char* dqm_label);

#endif // DATABASE_PENDING_H