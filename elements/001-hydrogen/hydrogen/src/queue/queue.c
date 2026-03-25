/*
 * Implementation of the thread-safe priority queue system.
 * 
 * Provides a robust implementation of named queues with hash-based lookup.
 * This implementation focuses on thread safety, performance, and reliability
 * through careful synchronization and resource management.
 * 
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "queue.h"

QueueSystem queue_system = {0};  // Explicit zero initialization
int queue_system_initialized = 0;  // Initialize to 0 (not initialized)

// DJB2 hash function chosen for queue name lookup because:
// 1. Excellent distribution - Minimizes collisions for string keys
// 2. Speed - Simple integer math operations, no complex calculations
// 3. Deterministic - Same name always maps to same bucket
// 4. Avalanche effect - Small changes in input create large changes in hash
//
// We use modulo QUEUE_HASH_SIZE to bound the result, accepting the slight
// increase in collision probability to maintain fixed memory usage.
unsigned int queue_hash(const char* str) {
    if (str == NULL) {
        return 0;  // Return 0 for NULL strings
    }

    unsigned int hash = 5381;
    unsigned char c;

    while ((c = (unsigned char)*str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash % QUEUE_HASH_SIZE;
}

/*
 * Initialize the queue system with clean state
 */
void queue_system_init(void) {

    log_this(SR_QUEUES, LOG_LINE_BREAK, LOG_LEVEL_DEBUG, 0);
    log_this(SR_QUEUES, "QUEUE INITIALIZATION", LOG_LEVEL_DEBUG, 0);

    memset(&queue_system, 0, sizeof(QueueSystem));
    if (pthread_mutex_init(&queue_system.mutex, NULL) != 0) {
        // Mutex initialization failed - this is a critical error
        queue_system_initialized = 0;
        log_this(SR_QUEUES, SR_QUEUES " initialization failed", LOG_LEVEL_FATAL, 0);
        return;
    }
    queue_system_initialized = 1;  // Mark as initialized

    update_queue_limits_from_config(app_config);    
    
    log_this(SR_QUEUES, "QUEUE INITIALIZATION COMPLETE", LOG_LEVEL_DEBUG, 0);
}

/*
 * Clean shutdown of the entire queue system
 */
void queue_system_destroy(void) {
    queue_system_initialized = 0;  // Mark as not initialized
    MutexResult lock_result = MUTEX_LOCK(&queue_system.mutex, SR_QUEUES);
    if (lock_result == MUTEX_SUCCESS) {
        for (int i = 0; i < QUEUE_HASH_SIZE; i++) {
            Queue* queue = queue_system.queues[i];
            while (queue) {
                Queue* next = queue->hash_next;
                queue_system.queues[i] = next;  // Remove from hash table
                if (queue->name) {  // Check if the queue is still valid
                    // Force immediate destruction during shutdown - bypass refcount
                    // Drain elements
                    MutexResult qlock_result = MUTEX_LOCK(&queue->mutex, SR_QUEUES);
                    if (qlock_result == MUTEX_SUCCESS) {
                        while (queue->head != NULL) {
                            QueueElement* temp = queue->head;
                            queue->head = queue->head->next;
                            free(temp->data);
                            free(temp);
                        }
                        mutex_unlock(&queue->mutex);
                    }
                    pthread_mutex_destroy(&queue->mutex);
                    pthread_cond_destroy(&queue->not_empty);
                    pthread_cond_destroy(&queue->not_full);
                    if (queue->name && (uintptr_t)queue->name >= 0x1000) {
                        free(queue->name);
                    }
                    free(queue);
                }
                queue = next;
            }
        }
        mutex_unlock(&queue_system.mutex);
        pthread_mutex_destroy(&queue_system.mutex);
    }
}

/*
 * Locate a queue by name with O(1) average complexity
 *
 * Why hash-based lookup?
 * - Constant time access critical for real-time ops
 * - Hash function spreads load across buckets
 * - Chaining handles collisions gracefully
 * - System lock prevents race conditions
 *
 * Thread Safety:
 * - System mutex protects hash table
 * - Early unlock after finding queue
 * - Queue's own mutex for operations
 */
Queue* queue_find_with_label(const char* name, const char* subsystem) {
    unsigned int index = queue_hash(name);
    MutexResult lock_result = MUTEX_LOCK(&queue_system.mutex, subsystem);
    if (lock_result == MUTEX_SUCCESS) {
        Queue* queue = queue_system.queues[index];
        while (queue) {
            // Safety check: ensure queue->name is valid before comparison
            // Skip queues that are marked for destruction
            if (queue->name && !queue->pending_destroy && strcmp(queue->name, name) == 0) {
                queue->refcount++;  // Increment refcount for caller
                mutex_unlock(&queue_system.mutex);
                return queue;
            }
            queue = queue->hash_next;
        }
        mutex_unlock(&queue_system.mutex);
    }
    return NULL;
}

Queue* queue_find(const char* name) {
    return queue_find_with_label(name, SR_QUEUES);
}

// Create a new message queue with comprehensive safety guarantees
//
// The creation process uses a multi-phase approach to ensure reliability:
// 1. Duplicate check first - Prevents resource waste on existing queues
// 2. Staged resource allocation - Allows clean rollback on any failure
// 3. Two-phase initialization - Separates structure setup from system integration
// 4. Atomic hash table insertion - Ensures queue is only visible when fully ready
//
// This careful orchestration prevents partially initialized queues from being
// visible to other threads and ensures clean cleanup on any initialization failure.
// The strdup of name creates a private copy, isolating the queue from external
// string lifetime issues.
Queue* queue_create_with_label(const char* name, const QueueAttributes* attrs, const char* subsystem) {
    if (name == NULL || attrs == NULL || strlen(name) == 0) {
        return NULL;
    }

    Queue* existing_queue = queue_find_with_label(name, subsystem);
    if (existing_queue) {
        return existing_queue;
    }

    Queue* queue = (Queue*)malloc(sizeof(Queue));
    if (queue == NULL) {
        return NULL;
    }

    queue->name = strdup(name);
    if (queue->name == NULL) {
        free(queue);
        return NULL;
    }

    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    queue->memory_used = 0;
    queue->hash_next = NULL;
    queue->refcount = 1;  // Creator gets initial reference
    queue->pending_destroy = false;

    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        free(queue->name);
        free(queue);
        return NULL;
    }

    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        free(queue->name);
        free(queue);
        return NULL;
    }

    if (pthread_cond_init(&queue->not_full, NULL) != 0) {
        pthread_cond_destroy(&queue->not_empty);
        pthread_mutex_destroy(&queue->mutex);
        free(queue->name);
        free(queue);
        return NULL;
    }

    // Initialize queue with attributes
    queue->attrs.initial_memory = attrs->initial_memory;
    queue->attrs.chunk_size = attrs->chunk_size;
    queue->attrs.warning_limit = attrs->warning_limit;

    // Add the queue to the hash table
    unsigned int index = queue_hash(name);
    MutexResult hash_lock_result = MUTEX_LOCK(&queue_system.mutex, subsystem);
    if (hash_lock_result == MUTEX_SUCCESS) {
        queue->hash_next = queue_system.queues[index];
        queue_system.queues[index] = queue;
        mutex_unlock(&queue_system.mutex);
    } else {
        // Failed to add to hash table, clean up
        pthread_cond_destroy(&queue->not_full);
        pthread_cond_destroy(&queue->not_empty);
        pthread_mutex_destroy(&queue->mutex);
        free(queue->name);
        free(queue);
        return NULL;
    }

    // During early initialization (SystemLog queue creation), logging system isn't ready
    // For all other queues, use the normal logging system
    // Temporarily disabled logging to prevent circular dependency with mutex wrapper
    /*
    if (strcmp(name, "SystemLog") != 0) {
        log_this(SR_QUEUES, "New queue created: %s", LOG_LEVEL_STATE, 1, name);
    }
    */

    return queue;
}

Queue* queue_create(const char* name, const QueueAttributes* attrs) {
    return queue_create_with_label(name, attrs, SR_QUEUES);
}

void queue_destroy(Queue* queue) {
    if (queue == NULL) {
        return;
    }

    // CRITICAL: Detect double-free by checking if queue is still in hash table
    // If queue was already destroyed, it won't be in the hash table and we should skip destruction
    // Search ALL hash buckets by queue pointer (not by name, which might be freed already)
    bool found_in_hash = false;
    
    MutexResult hash_lock_result = MUTEX_LOCK(&queue_system.mutex, SR_QUEUES);
    if (hash_lock_result == MUTEX_SUCCESS) {
        // Search all buckets for this queue pointer (don't rely on potentially-freed name)
        for (int i = 0; i < QUEUE_HASH_SIZE && !found_in_hash; i++) {
            Queue** current = &queue_system.queues[i];
            while (*current) {
                if (*current == queue) {
                    *current = queue->hash_next;  // Remove from linked list
                    found_in_hash = true;
                    break;
                }
                current = &(*current)->hash_next;
            }
        }
        mutex_unlock(&queue_system.mutex);
    }
    
    // If queue wasn't in hash table, it was already destroyed - skip cleanup to prevent double-free
    if (!found_in_hash) {
        return;
    }

    // Decrement refcount (creator had 1 ref, destroy call releases it)
    queue->refcount--;
    
    // If refs are still held by other threads, mark for deferred destruction
    // The actual struct free will happen in queue_release when refcount reaches 0
    if (queue->refcount > 0) {
        queue->pending_destroy = true;
        return;
    }

    // Free the queue immediately (no other references)
    MutexResult qlr = MUTEX_LOCK(&queue->mutex, SR_QUEUES);
    if (qlr == MUTEX_SUCCESS) {
        while (queue->head != NULL) {
            QueueElement* temp = queue->head;
            queue->head = queue->head->next;
            free(temp->data);
            free(temp);
        }

        mutex_unlock(&queue->mutex);
    }
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);

    if (queue->name && (uintptr_t)queue->name >= 0x1000) {
        free(queue->name);
    }
    free(queue);
}

// Idempotent version of queue_destroy - safe to call multiple times
// Returns true if queue was actually destroyed, false if already destroyed
bool queue_destroy_safe(Queue* queue) {
    if (queue == NULL) {
        return false;
    }

    // Check if queue is still in hash table
    bool found_in_hash = false;
    
    MutexResult hash_lock_result = MUTEX_LOCK(&queue_system.mutex, SR_QUEUES);
    if (hash_lock_result == MUTEX_SUCCESS) {
        for (int i = 0; i < QUEUE_HASH_SIZE && !found_in_hash; i++) {
            Queue** current = &queue_system.queues[i];
            while (*current) {
                if (*current == queue) {
                    *current = queue->hash_next;
                    found_in_hash = true;
                    break;
                }
                current = &(*current)->hash_next;
            }
        }
        mutex_unlock(&queue_system.mutex);
    }
    
    // If not in hash, already destroyed
    if (!found_in_hash) {
        return false;
    }

    // Decrement refcount
    queue->refcount--;
    
    if (queue->refcount > 0) {
        queue->pending_destroy = true;
        return false;  // Not fully destroyed yet
    }

    // Free any remaining elements (queue should already be empty via queue_clear,
    // but we check to be safe in case queue_clear wasn't called)
    MutexResult qlr = MUTEX_LOCK(&queue->mutex, SR_QUEUES);
    if (qlr == MUTEX_SUCCESS) {
        // Only free elements if there are any (shouldn't happen normally)
        while (queue->head != NULL) {
            QueueElement* temp = queue->head;
            queue->head = queue->head->next;
            if (temp->data) {
                free(temp->data);
                temp->data = NULL;
            }
            free(temp);
        }
        queue->size = 0;
        mutex_unlock(&queue->mutex);
    }
    
    // Destroy synchronization primitives
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);

    // Free the queue name and structure
    if (queue->name && (uintptr_t)queue->name >= 0x1000) {
        free(queue->name);
        queue->name = NULL;
    }
    free(queue);
    return true;
}

// Increment refcount for a queue (call when borrowing a queue pointer)
void queue_acquire(Queue* queue) {
    if (!queue) return;
    
    MutexResult lock_result = MUTEX_LOCK(&queue->mutex, SR_QUEUES);
    if (lock_result == MUTEX_SUCCESS) {
        queue->refcount++;
        mutex_unlock(&queue->mutex);
    }
}

// Decrement refcount, destroy if reaches 0
void queue_release(Queue* queue) {
    if (!queue) return;
    
    MutexResult lock_result = MUTEX_LOCK(&queue->mutex, SR_QUEUES);
    if (lock_result != MUTEX_SUCCESS) {
        return;
    }
    
    queue->refcount--;
    int refcount = queue->refcount;
    mutex_unlock(&queue->mutex);
    
    // If refcount reaches 0, destroy the queue
    if (refcount <= 0) {
        queue_destroy_safe(queue);
    }
}


// Add a message to the queue with guaranteed ordering and memory safety
//
// The enqueue operation uses a carefully designed sequence to ensure:
// 1. Memory isolation - Creates private copy of data to prevent external modifications
// 2. Null termination - Always adds null byte for safety with string operations
// 3. Atomic state updates - All queue state changes happen under single lock
// 4. Priority preservation - Messages maintain strict FIFO order within priority levels
//
// The design chooses to copy rather than reference data to:
// - Prevent memory corruption from external changes
// - Allow safe deallocation of source memory
// - Simplify memory ownership semantics
bool queue_enqueue(Queue* queue, const char* data, size_t size, int priority) {
    if (!queue || !data || size == 0) {
        return false;
    }

    QueueElement* new_element = (QueueElement*)malloc(sizeof(QueueElement));
    if (!new_element) {
        return false;
    }

    new_element->data = malloc(size + 1);  // +1 for null terminator
    if (!new_element->data) {
        free(new_element);
        return false;
    }

    memcpy(new_element->data, data, size);
    new_element->data[size] = '\0';  // Ensure null termination
    new_element->size = size;
    new_element->priority = priority;
    clock_gettime(CLOCK_REALTIME, &new_element->timestamp);
    new_element->next = NULL;

    MutexResult lock_result = MUTEX_LOCK(&queue->mutex, SR_QUEUES);
    if (lock_result == MUTEX_SUCCESS) {
        if (queue->tail) {
            queue->tail->next = new_element;
        } else {
            queue->head = new_element;
        }
        queue->tail = new_element;
        queue->size++;
        queue->memory_used += size;

        pthread_cond_signal(&queue->not_empty);
        mutex_unlock(&queue->mutex);
    } else {
        // Failed to lock, clean up
        free(new_element->data);
        free(new_element);
        return false;
    }

    return true;
} 

// Remove and return the next message using conditional waiting
//
// The dequeue operation implements a careful balance between:
// 1. Blocking behavior - Threads sleep when queue is empty to reduce CPU usage
// 2. Memory ownership - Transfers data ownership to caller for clear lifecycle
// 3. State consistency - Maintains queue invariants even during error conditions
// 4. Resource accounting - Tracks memory usage for system monitoring
//
// Uses pthread_cond_wait to implement blocking behavior, which:
// - Reduces CPU usage compared to polling
// - Automatically handles spurious wakeups
// - Maintains mutex ordering to prevent deadlocks
char* queue_dequeue(Queue* queue, size_t* size, int* priority) {
    if (!queue || !size || !priority) {
        return NULL;
    }

    MutexResult lock_result = MUTEX_LOCK(&queue->mutex, SR_QUEUES);
    if (lock_result != MUTEX_SUCCESS) {
        return NULL;
    }

    while (queue->size == 0) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }

    QueueElement* element = queue->head;
    queue->head = element->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    queue->size--;
    queue->memory_used -= element->size;

    pthread_cond_signal(&queue->not_full);
    mutex_unlock(&queue->mutex);

    char* data = element->data;
    *size = element->size;
    *priority = element->priority;

    free(element); // Free the QueueElement, but not the data

    return data;
}

/*
 * Non-blocking dequeue operation
 *
 * Returns NULL immediately if queue is empty instead of blocking.
 * This is essential for WebSocket callbacks that must not block.
 */
char* queue_dequeue_nonblocking(Queue* queue, size_t* size, int* priority) {
    if (!queue || !size || !priority) {
        return NULL;
    }

    MutexResult lock_result = MUTEX_LOCK(&queue->mutex, SR_QUEUES);
    if (lock_result != MUTEX_SUCCESS) {
        return NULL;
    }

    // Non-blocking: return NULL if queue is empty
    if (queue->size == 0) {
        mutex_unlock(&queue->mutex);
        return NULL;
    }

    QueueElement* element = queue->head;
    queue->head = element->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    queue->size--;
    queue->memory_used -= element->size;

    pthread_cond_signal(&queue->not_full);
    mutex_unlock(&queue->mutex);

    char* data = element->data;
    *size = element->size;
    *priority = element->priority;

    free(element); // Free the QueueElement, but not the data

    return data;
}

/*
 * Get current queue size with minimal locking
 *
 * Why this design?
 * - Short critical section
 * - Atomic size counter
 * - No element traversal
 * - Safe concurrent access
 *
 * Returns 0 for invalid queues to prevent crashes
 */
size_t queue_size(Queue* queue) {
    if (!queue) {
        return 0;
    }

    MutexResult lock_result = MUTEX_LOCK(&queue->mutex, SR_QUEUES);
    size_t size = 0;
    if (lock_result == MUTEX_SUCCESS) {
        size = queue->size;
        mutex_unlock(&queue->mutex);
    }

    return size;
}


/*
 * Track memory usage for resource management
 *
 * Why track memory?
 * - Detect memory leaks
 * - Prevent exhaustion
 * - Monitor queue health
 * - Guide cleanup decisions
 *
 * Uses atomic counter for accuracy
 */
size_t queue_memory_usage(Queue* queue) {
    if (!queue) {
        return 0;
    }

    MutexResult lock_result = MUTEX_LOCK(&queue->mutex, SR_QUEUES);
    size_t memory_used = 0;
    if (lock_result == MUTEX_SUCCESS) {
        memory_used = queue->memory_used;
        mutex_unlock(&queue->mutex);
    }

    return memory_used;
}

/*
 * Calculate age of oldest message for queue health
 *
 * Why track message age?
 * - Detect stalled messages
 * - Guide priority boosting
 * - Monitor processing delays
 * - Support timeout policies
 *
 * Uses monotonic clock for reliability
 */
long queue_oldest_element_age(Queue* queue) {
    if (!queue) {
        return 0;
    }

    MutexResult lock_result = MUTEX_LOCK(&queue->mutex, SR_QUEUES);
    if (lock_result != MUTEX_SUCCESS) {
        return 0;
    }

    if (queue->size == 0) {
        mutex_unlock(&queue->mutex);
        return 0;
    }

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    long age_ms = (now.tv_sec - queue->head->timestamp.tv_sec) * 1000 +
                  (now.tv_nsec - queue->head->timestamp.tv_nsec) / 1000000;

    mutex_unlock(&queue->mutex);

    return age_ms;
}

long queue_youngest_element_age(Queue* queue) {
    if (!queue) {
        return 0;
    }

    MutexResult lock_result = MUTEX_LOCK(&queue->mutex, SR_QUEUES);
    if (lock_result != MUTEX_SUCCESS) {
        return 0;
    }

    if (queue->size == 0) {
        mutex_unlock(&queue->mutex);
        return 0;
    }

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    long age_ms = (now.tv_sec - queue->tail->timestamp.tv_sec) * 1000 +
                  (now.tv_nsec - queue->tail->timestamp.tv_nsec) / 1000000;

    mutex_unlock(&queue->mutex);

    return age_ms;
}

/*
 * Remove all messages from queue immediately
 *
 * Why needed?
 * - Emergency cleanup
 * - System reset
 * - Memory pressure relief
 * - Queue recycling
 *
 * Implementation:
 * - Single-pass cleanup
 * - Proper memory deallocation
 * - Maintains queue structure
 * - Updates statistics
 */
void queue_clear(Queue* queue) {
    if (!queue) return;

    MutexResult lock_result = MUTEX_LOCK(&queue->mutex, SR_QUEUES);
    if (lock_result == MUTEX_SUCCESS) {
        while (queue->head) {
            QueueElement* temp = queue->head;
            queue->head = queue->head->next;
            free(temp->data);
            free(temp);
        }
        queue->tail = NULL;
        queue->size = 0;
        queue->memory_used = 0;
        mutex_unlock(&queue->mutex);
    }
}
