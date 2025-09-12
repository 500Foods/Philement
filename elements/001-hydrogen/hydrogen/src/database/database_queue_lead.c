/*
 * Database Queue Lead Management Functions
 *
 * Implements Lead queue specific functions for the Hydrogen database subsystem.
 * Split from database_queue.c for better maintainability.
 */

#include "../hydrogen.h"
#include <assert.h>

// Local includes
#include "database_queue.h"
#include "database.h"

/*
 * Spawn a child queue of the specified type
 */
bool database_queue_spawn_child_queue(DatabaseQueue* lead_queue, const char* queue_type) {
    if (!lead_queue || !lead_queue->is_lead_queue || !queue_type) {
        return false;
    }

    pthread_mutex_lock(&lead_queue->children_lock);

    // Check if we already have a child queue of this type
    for (int i = 0; i < lead_queue->child_queue_count; i++) {
        if (lead_queue->child_queues[i] &&
            lead_queue->child_queues[i]->queue_type &&
            strcmp(lead_queue->child_queues[i]->queue_type, queue_type) == 0) {
            pthread_mutex_unlock(&lead_queue->children_lock);
            // log_this(SR_DATABASE, "Child queue %s already exists for database %s", LOG_LEVEL_DEBUG, 2 queue_type, lead_queue->database_name);
            return true; // Already exists
        }
    }

    // Check if we have space for more child queues
    if (lead_queue->child_queue_count >= lead_queue->max_child_queues) {
        pthread_mutex_unlock(&lead_queue->children_lock);
        // log_this(SR_DATABASE, "Cannot spawn %s queue: maximum child queues reached for %s", LOG_LEVEL_ERROR, 2 queue_type, lead_queue->database_name);
        return false;
    }

    // Create the child worker queue
    DatabaseQueue* child_queue = database_queue_create_worker(
        lead_queue->database_name,
        lead_queue->connection_string,
        queue_type
    );

    if (!child_queue) {
        pthread_mutex_unlock(&lead_queue->children_lock);
        char* dqm_label = database_queue_generate_label(lead_queue);
        log_this(dqm_label, "Failed to create child queue", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
        return false;
    }

    // Assign queue number (find next available number)
    int next_queue_number = 1;  // Start from 01 (Lead is 00)
    bool number_taken = true;
    while (number_taken) {
        number_taken = false;
        for (int i = 0; i < lead_queue->child_queue_count; i++) {
            if (lead_queue->child_queues[i] &&
                lead_queue->child_queues[i]->queue_number == next_queue_number) {
                number_taken = true;
                next_queue_number++;
                break;
            }
        }
    }
    child_queue->queue_number = next_queue_number;

    // Start the worker thread for the child queue
    if (!database_queue_start_worker(child_queue)) {
        pthread_mutex_unlock(&lead_queue->children_lock);
        database_queue_destroy(child_queue);
        char* dqm_label = database_queue_generate_label(lead_queue);
        log_this(dqm_label, "Failed to start worker for child queue", LOG_LEVEL_ERROR, 0);
        free(dqm_label);
        return false;
    }

    // Add to child queue array
    lead_queue->child_queues[lead_queue->child_queue_count] = child_queue;
    lead_queue->child_queue_count++;

    // Update Lead queue tags - remove the tag that was assigned to child
    char tag_to_remove = '\0';
    if (strcmp(queue_type, QUEUE_TYPE_SLOW) == 0) tag_to_remove = 'S';
    else if (strcmp(queue_type, QUEUE_TYPE_MEDIUM) == 0) tag_to_remove = 'M';
    else if (strcmp(queue_type, QUEUE_TYPE_FAST) == 0) tag_to_remove = 'F';
    else if (strcmp(queue_type, QUEUE_TYPE_CACHE) == 0) tag_to_remove = 'C';

    if (tag_to_remove != '\0') {
        database_queue_remove_tag(lead_queue, tag_to_remove);
    }

    pthread_mutex_unlock(&lead_queue->children_lock);

    char* dqm_label = database_queue_generate_label(lead_queue);
    log_this(dqm_label, "Spawned child queue", LOG_LEVEL_STATE, 0);
    free(dqm_label);
    return true;
}

/*
 * Shutdown a child queue of the specified type
 */
bool database_queue_shutdown_child_queue(DatabaseQueue* lead_queue, const char* queue_type) {
    if (!lead_queue || !lead_queue->is_lead_queue || !queue_type) {
        return false;
    }

    pthread_mutex_lock(&lead_queue->children_lock);

    // Find the child queue to shutdown
    int target_index = -1;
    for (int i = 0; i < lead_queue->child_queue_count; i++) {
        if (lead_queue->child_queues[i] &&
            lead_queue->child_queues[i]->queue_type &&
            strcmp(lead_queue->child_queues[i]->queue_type, queue_type) == 0) {
            target_index = i;
            break;
        }
    }

    if (target_index == -1) {
        pthread_mutex_unlock(&lead_queue->children_lock);
        // log_this(SR_DATABASE, "Child queue %s not found for database %s", LOG_LEVEL_DEBUG, 2, queue_type, lead_queue->database_name);
        return false;
    }

    // Destroy the child queue
    DatabaseQueue* child_queue = lead_queue->child_queues[target_index];
    database_queue_destroy(child_queue);

    // Compact the array by moving the last element to the removed position
    if (target_index < lead_queue->child_queue_count - 1) {
        lead_queue->child_queues[target_index] = lead_queue->child_queues[lead_queue->child_queue_count - 1];
    }
    lead_queue->child_queues[lead_queue->child_queue_count - 1] = NULL;
    lead_queue->child_queue_count--;

    pthread_mutex_unlock(&lead_queue->children_lock);

    log_this(SR_DATABASE, "Shutdown %s child queue for database %s", LOG_LEVEL_STATE, 2, queue_type, lead_queue->database_name);
    return true;
}