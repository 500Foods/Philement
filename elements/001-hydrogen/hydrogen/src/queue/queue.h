/*
 * Thread-safe priority queue system for message passing.
 * 
  */

#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include <stdbool.h>
#include <time.h>

/*
 * Queue System Constants
 * 
 * QUEUE_HASH_SIZE: 256 chosen because:
 * - Power of 2 for efficient modulo
 * - Balances memory use vs collision rate
 * - Sufficient for typical printer needs
 * - Small enough for embedded systems
 */
#define QUEUE_HASH_SIZE 256

/*
 * Queue Element Structure
 * 
 * Why this design?
 * - Separate data/size for variable messages
 * - Priority field enables urgent handling
 * - Timestamp enables age-based policies
 * - Next pointer supports efficient linking
 * 
 * Memory Layout:
 * - Aligned for efficient access
 * - Minimized padding waste
 * - Cache-friendly ordering
 */
typedef struct QueueElement {
    char* data;                     // Message data buffer
    size_t size;                    // Size of data in bytes
    int priority;                   // Message priority (higher = more urgent)
    struct timespec timestamp;      // Message creation time
    struct QueueElement* next;      // Next element in queue
} QueueElement;

/*
 * Queue Configuration
 *
 */
typedef struct QueueAttributes {
    size_t initial_memory;  // Initial memory allocation
    size_t chunk_size;      // Memory allocation granularity
    size_t warning_limit;   // Memory usage warning threshold
} QueueAttributes;

/*
 * Queue Structure
 *
 */
typedef struct Queue {
    char* name;                                     // Queue identifier
    QueueElement* head;                             // First element in queue
    QueueElement* tail;                             // Last element in queue
    size_t size;                                    // Number of elements
    size_t memory_used;                             // Total memory usage
    pthread_mutex_t mutex;                          // Queue lock
    pthread_cond_t not_empty;                       // Signaled when queue becomes non-empty
    pthread_cond_t not_full;                        // Signaled when queue has space
    struct Queue* hash_next;                        // Next queue in hash chain
    int highest_priority;                           // Highest current priority
    struct timespec oldest_element_timestamp;       // Oldest message time
    struct timespec youngest_element_timestamp;     // Newest message time
    QueueAttributes attrs;                          // Queue configuration attributes
} Queue;

/*
 * Queue System State
 * 
 * Why this structure?
 * - Fixed-size array bounds memory use
 * - Single mutex for system operations
 * - Simple iteration for cleanup
 * - Efficient queue lookup
 */
typedef struct QueueSystem {
    Queue* queues[QUEUE_HASH_SIZE];  // Hash table of queues
    pthread_mutex_t mutex;           // System-wide lock
} QueueSystem;

extern QueueSystem queue_system;
extern int queue_system_initialized;  // Flag indicating if queue system is ready

/*
 * Queue System Interface
 *
 */

// Hash Function
unsigned int queue_hash(const char* str);  // DJB2 hash function for queue name lookup

// System Lifecycle
void queue_system_init(void);    // Initialize queue system
void queue_system_destroy(void); // Clean shutdown of all queues

// Queue Management
Queue* queue_find(const char* name);  // O(1) queue lookup
Queue* queue_find_with_label(const char* name, const char* subsystem);  // O(1) queue lookup with custom subsystem
Queue* queue_create(const char* name, const QueueAttributes* attrs);  // Create/get queue
Queue* queue_create_with_label(const char* name, const QueueAttributes* attrs, const char* subsystem);  // Create/get queue with custom subsystem
void queue_destroy(Queue* queue);  // Clean up queue resources

// Message Operations
bool queue_enqueue(Queue* queue, const char* data, size_t size, int priority);  // Add message
char* queue_dequeue(Queue* queue, size_t* size, int* priority);  // Remove message

// Monitoring
size_t queue_size(Queue* queue);  // Current message count
size_t queue_memory_usage(Queue* queue);  // Current memory use
long queue_oldest_element_age(Queue* queue);  // Oldest message age
long queue_youngest_element_age(Queue* queue);  // Newest message age

// Maintenance
void queue_clear(Queue* queue);  // Remove all messages

#endif // QUEUE_H
