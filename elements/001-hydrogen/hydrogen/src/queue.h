/*
 * Thread-safe priority queue system for message passing.
 * 
 * Implements a high-performance, multi-queue message passing system using
 * a hash-based lookup for queue management. The system provides thread-safe
 * operations with priority support and memory tracking.
 * 
 * Queue System Architecture:
 * - Hash table for fast queue lookup by name
 * - Linked list for collision handling
 * - Per-queue mutex for fine-grained locking
 * - System-wide mutex for queue creation/deletion
 * 
 * Thread Safety:
 * - Mutex protection for all operations
 * - Condition variables for blocking operations
 * - Lock-free reads where possible
 * - Atomic priority updates
 * 
 * Memory Management:
 * - Per-queue memory tracking
 * - Configurable memory limits
 * - Warning thresholds
 * - Automatic cleanup
 * 
 * Priority System:
 * - Multiple priority levels
 * - FIFO within same priority
 * - Priority inheritance
 * - Priority boost mechanism
 * 
 * Performance Features:
 * - O(1) queue lookup
 * - O(1) enqueue/dequeue
 * - Memory pooling
 * - Batch operations
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#define QUEUE_HASH_SIZE 256

// Queue element structure
// Represents a single message in the queue with:
// - Data buffer and size
// - Priority level
// - Timestamp for age tracking
// - Next pointer for linked list
typedef struct QueueElement {
    char* data;              // Message data buffer
    size_t size;            // Size of data in bytes
    int priority;           // Message priority (higher = more urgent)
    struct timespec timestamp;  // Message creation time
    struct QueueElement* next;  // Next element in queue
} QueueElement;

// Queue structure
// Manages a priority-based message queue with:
// - Thread synchronization primitives
// - Memory tracking
// - Statistics collection
// - Hash table chaining
typedef struct Queue {
    char* name;              // Queue identifier
    QueueElement* head;      // First element in queue
    QueueElement* tail;      // Last element in queue
    size_t size;            // Number of elements
    size_t memory_used;     // Total memory usage
    pthread_mutex_t mutex;   // Queue lock
    pthread_cond_t not_empty;  // Signaled when queue becomes non-empty
    pthread_cond_t not_full;   // Signaled when queue has space
    struct Queue* hash_next;   // Next queue in hash chain
    int highest_priority;    // Highest current priority
    struct timespec oldest_element_timestamp;   // Oldest message time
    struct timespec youngest_element_timestamp; // Newest message time
} Queue;

// Queue configuration attributes
// Defines memory and performance parameters:
// - Memory allocation strategy
// - Warning thresholds
// - Performance tuning
typedef struct QueueAttributes {
    size_t initial_memory;  // Initial memory allocation
    size_t chunk_size;      // Memory allocation granularity
    size_t warning_limit;   // Memory usage warning threshold
    // Add more attributes as needed
} QueueAttributes;

typedef struct QueueSystem {
    Queue* queues[QUEUE_HASH_SIZE];
    pthread_mutex_t mutex;
} QueueSystem;

extern QueueSystem queue_system;

void queue_system_init();
void queue_system_destroy();
Queue* queue_find(const char* name);
Queue* queue_create(const char* name, QueueAttributes* attrs);
void queue_destroy(Queue* queue);
bool queue_enqueue(Queue* queue, const char* data, size_t size, int priority);
char* queue_dequeue(Queue* queue, size_t* size, int* priority);
size_t queue_size(Queue* queue);
size_t queue_memory_usage(Queue* queue);
long queue_oldest_element_age(Queue* queue);
long queue_youngest_element_age(Queue* queue);
void queue_clear(Queue* queue);

#endif // QUEUE_H
