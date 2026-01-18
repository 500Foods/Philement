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
} PendingQueryResult;

/**
 * @brief Manager for all pending query results
 *
 * Thread-safe container for tracking all active pending results.
 */
typedef struct PendingResultManager {
    PendingQueryResult** results;      /**< Array of pending results */
    size_t count;                      /**< Current number of results */
    size_t capacity;                   /**< Allocated capacity */
    pthread_mutex_t manager_lock;      /**< Protects result array */
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

#endif // DATABASE_PENDING_H