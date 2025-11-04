/*
 * Hydrogen Database Pending Results Implementation
 * Synchronous query execution with timeout support
 */

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "database_pending.h"
#include "database_types.h"

// Global pending result manager instance
static PendingResultManager* g_pending_manager = NULL;

/**
 * @brief Initialize the pending result manager
 */
PendingResultManager* pending_result_manager_create(const char* dqm_label) {
    PendingResultManager* manager = calloc(1, sizeof(PendingResultManager));
    if (!manager) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "Failed to allocate pending result manager", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Initialize with reasonable capacity
    manager->capacity = 64;
    manager->results = calloc(manager->capacity, sizeof(PendingQueryResult*));
    if (!manager->results) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "Failed to allocate pending results array", LOG_LEVEL_ERROR, 0);
        free(manager);
        return NULL;
    }

    // Initialize mutex
    if (pthread_mutex_init(&manager->manager_lock, NULL) != 0) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "Failed to initialize manager mutex", LOG_LEVEL_ERROR, 0);
        free(manager->results);
        free(manager);
        return NULL;
    }

    log_this(dqm_label ? dqm_label : SR_DATABASE, "Pending result manager created", LOG_LEVEL_DEBUG, 0);
    return manager;
}

/**
 * @brief Destroy the pending result manager and all pending results
 */
void pending_result_manager_destroy(PendingResultManager* manager, const char* dqm_label) {
    if (!manager) return;

    pthread_mutex_lock(&manager->manager_lock);

    // Clean up all pending results
    for (size_t i = 0; i < manager->count; i++) {
        PendingQueryResult* pending = manager->results[i];
        if (pending) {
            // Clean up the pending result
            if (pending->query_id) free(pending->query_id);
            if (pending->result) {
                // TODO: Add proper QueryResult cleanup when implemented
                free(pending->result);
            }
            pthread_mutex_destroy(&pending->result_lock);
            pthread_cond_destroy(&pending->result_ready);
            free(pending);
        }
    }

    free(manager->results);
    pthread_mutex_unlock(&manager->manager_lock);
    pthread_mutex_destroy(&manager->manager_lock);
    free(manager);

    log_this(dqm_label ? dqm_label : SR_DATABASE, "Pending result manager destroyed", LOG_LEVEL_DEBUG, 0);
}

/**
 * @brief Register a new pending result
 */
PendingQueryResult* pending_result_register(
    PendingResultManager* manager,
    const char* query_id,
    int timeout_seconds,
    const char* dqm_label
) {
    if (!manager || !query_id) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "Invalid parameters for pending result registration", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    PendingQueryResult* pending = calloc(1, sizeof(PendingQueryResult));
    if (!pending) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "Failed to allocate pending result", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Copy query ID
    pending->query_id = strdup(query_id);
    if (!pending->query_id) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "Failed to copy query ID", LOG_LEVEL_ERROR, 0);
        free(pending);
        return NULL;
    }

    pending->timeout_seconds = timeout_seconds;
    pending->submitted_at = time(NULL);
    pending->completed = false;
    pending->timed_out = false;

    // Initialize synchronization primitives
    if (pthread_mutex_init(&pending->result_lock, NULL) != 0) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "Failed to initialize result mutex", LOG_LEVEL_ERROR, 0);
        free(pending->query_id);
        free(pending);
        return NULL;
    }

    if (pthread_cond_init(&pending->result_ready, NULL) != 0) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "Failed to initialize result condition", LOG_LEVEL_ERROR, 0);
        pthread_mutex_destroy(&pending->result_lock);
        free(pending->query_id);
        free(pending);
        return NULL;
    }

    // Add to manager
    pthread_mutex_lock(&manager->manager_lock);

    // Expand array if needed
    if (manager->count >= manager->capacity) {
        size_t new_capacity = manager->capacity * 2;
        PendingQueryResult** new_results = realloc(manager->results, new_capacity * sizeof(PendingQueryResult*));
        if (!new_results) {
            log_this(dqm_label ? dqm_label : SR_DATABASE, "Failed to expand pending results array", LOG_LEVEL_ERROR, 0);
            pthread_mutex_unlock(&manager->manager_lock);
            pthread_cond_destroy(&pending->result_ready);
            pthread_mutex_destroy(&pending->result_lock);
            free(pending->query_id);
            free(pending);
            return NULL;
        }
        manager->results = new_results;
        manager->capacity = new_capacity;
    }

    manager->results[manager->count++] = pending;
    pthread_mutex_unlock(&manager->manager_lock);

    log_this(dqm_label ? dqm_label : SR_DATABASE, "Pending result registered", LOG_LEVEL_DEBUG, 0);
    return pending;
}

/**
 * @brief Wait for a pending result to complete
 */
int pending_result_wait(PendingQueryResult* pending, const char* dqm_label) {
    if (!pending) {
        return -1;
    }

    struct timespec timeout;
    // Use gettimeofday for timeout calculation (more portable)
    struct timeval tv;
    gettimeofday(&tv, NULL);
    timeout.tv_sec = tv.tv_sec + pending->timeout_seconds;
    timeout.tv_nsec = tv.tv_usec * 1000;

    pthread_mutex_lock(&pending->result_lock);

    while (!pending->completed && !pending->timed_out) {
        int rc = pthread_cond_timedwait(
            &pending->result_ready,
            &pending->result_lock,
            &timeout
        );

        if (rc == ETIMEDOUT) {
            pending->timed_out = true;
            log_this(dqm_label ? dqm_label : SR_DATABASE, "Query timeout occurred", LOG_LEVEL_ERROR, 0);
            break;
        } else if (rc != 0) {
            log_this(dqm_label ? dqm_label : SR_DATABASE, "Error waiting for query result", LOG_LEVEL_ERROR, 0);
            pthread_mutex_unlock(&pending->result_lock);
            return -1;
        }
    }

    pthread_mutex_unlock(&pending->result_lock);

    if (pending->timed_out) {
        return -1;
    }

    return 0;
}

/**
 * @brief Signal that a query result is ready
 */
bool pending_result_signal_ready(
    PendingResultManager* manager,
    const char* query_id,
    QueryResult* result,
    const char* dqm_label
) {
    if (!manager || !query_id) {
        return false;
    }

    bool found = false;

    pthread_mutex_lock(&manager->manager_lock);

    // Find the pending result
    for (size_t i = 0; i < manager->count; i++) {
        PendingQueryResult* pending = manager->results[i];
        if (pending && strcmp(pending->query_id, query_id) == 0) {
            // Found it - signal completion
            pthread_mutex_lock(&pending->result_lock);
            pending->result = result;  // Transfer ownership
            pending->completed = true;
            pthread_cond_signal(&pending->result_ready);
            pthread_mutex_unlock(&pending->result_lock);

            found = true;
            break;
        }
    }

    pthread_mutex_unlock(&manager->manager_lock);

    if (found) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "Query result signaled as ready", LOG_LEVEL_DEBUG, 0);
    } else {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "Query result not found for signaling", LOG_LEVEL_ERROR, 0);
        // Clean up the result if not claimed - but don't free it here as caller owns it
        // The caller (test) will handle cleanup
    }

    return found;
}

/**
 * @brief Get the result from a completed pending query
 */
QueryResult* pending_result_get(const PendingQueryResult* pending) {
    if (!pending || !pending->completed || !pending->result) {
        return NULL;
    }
    return pending->result;
}

/**
 * @brief Check if a pending result has completed
 */
bool pending_result_is_completed(const PendingQueryResult* pending) {
    if (!pending) return false;
    return pending->completed;
}

/**
 * @brief Check if a pending result has timed out
 */
bool pending_result_is_timed_out(const PendingQueryResult* pending) {
    if (!pending) return false;
    return pending->timed_out;
}

/**
 * @brief Clean up expired pending results
 */
size_t pending_result_cleanup_expired(PendingResultManager* manager, const char* dqm_label) {
    if (!manager) return 0;

    size_t cleaned = 0;
    time_t now = time(NULL);

    pthread_mutex_lock(&manager->manager_lock);

    // Iterate backwards to safely remove elements
    for (size_t i = manager->count; i > 0; i--) {
        size_t index = i - 1;
        PendingQueryResult* pending = manager->results[index];

        if (pending) {
            time_t elapsed = now - pending->submitted_at;
            bool expired = (elapsed >= pending->timeout_seconds);

            if (expired || pending->timed_out) {
                // Remove from array
                manager->results[index] = NULL;

                // Clean up the pending result
                if (pending->query_id) free(pending->query_id);
                if (pending->result) {
                    // TODO: Add proper QueryResult cleanup when implemented
                    free(pending->result);
                }
                pthread_mutex_destroy(&pending->result_lock);
                pthread_cond_destroy(&pending->result_ready);
                free(pending);

                cleaned++;

                // Shift remaining elements
                for (size_t j = index; j < manager->count - 1; j++) {
                    manager->results[j] = manager->results[j + 1];
                }
                manager->count--;
            }
        }
    }

    pthread_mutex_unlock(&manager->manager_lock);

    if (cleaned > 0) {
        log_this(dqm_label ? dqm_label : SR_DATABASE, "Cleaned up expired pending results", LOG_LEVEL_DEBUG, 0);
    }

    return cleaned;
}

/**
 * @brief Get the global pending result manager instance
 */
PendingResultManager* get_pending_result_manager(void) {
    if (!g_pending_manager) {
        // Initialize global manager on first access
        g_pending_manager = pending_result_manager_create(NULL);
    }
    return g_pending_manager;
}