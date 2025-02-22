/*
 * Thread-safe priority queue system for message passing.
 * 
 * This module provides a high-performance message passing system designed for
 * the unique needs of a 3D printer control system. Key design decisions:
 * 
 * 1. Multi-Queue Architecture
 *    Why hash-based lookup?
 *    - O(1) queue access for real-time requirements
 *    - Separate queues isolate different subsystems
 *    - Named queues enable dynamic creation/deletion
 *    - Hash collisions handled via chaining for reliability
 * 
 * 2. Thread Safety Strategy
 *    Why fine-grained locking?
 *    - Per-queue mutexes minimize contention
 *    - System mutex only for queue lifecycle
 *    - Condition variables for efficient blocking
 *    - Lock-free reads where safe for performance
 * 
 * 3. Memory Management
 *    Why tracking and limits?
 *    - Prevents memory exhaustion in embedded systems
 *    - Early warning system for memory issues
 *    - Per-queue accounting for isolation
 *    - Automatic cleanup of abandoned queues
 * 
 * 4. Priority Handling
 *    Why priority queues?
 *    - Critical messages (e.g., emergency stop) need immediate handling
 *    - FIFO within priority levels maintains order
 *    - Priority inheritance prevents starvation
 *    - Dynamic priority adjustment for aging messages
 * 
 * 5. Performance Optimizations
 *    Why these specific choices?
 *    - O(1) operations critical for real-time response
 *    - Memory pooling reduces allocation overhead
 *    - Batch operations amortize lock overhead
 *    - Minimal copying through reference counting
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
    char* data;              // Message data buffer
    size_t size;            // Size of data in bytes
    int priority;           // Message priority (higher = more urgent)
    struct timespec timestamp;  // Message creation time
    struct QueueElement* next;  // Next element in queue
} QueueElement;

/*
 * Queue Structure
 * 
 * Design Considerations:
 * 1. Thread Safety
 *    - Mutex protects all state changes
 *    - Condition vars for blocking ops
 *    - Atomic updates where possible
 * 
 * 2. Performance
 *    - Head/tail pointers for O(1) ops
 *    - Size tracking avoids counting
 *    - Memory usage for monitoring
 * 
 * 3. Features
 *    - Named queues for identification
 *    - Priority tracking for urgency
 *    - Timestamp tracking for aging
 *    - Hash chaining for collisions
 */
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

/*
 * Queue Configuration
 * 
 * Why configurable attributes?
 * 1. Memory Control
 *    - Initial size prevents fragmentation
 *    - Chunk size optimizes allocation
 *    - Warning limits prevent exhaustion
 * 
 * 2. Performance Tuning
 *    - Customizable per queue type
 *    - Adjustable based on usage
 *    - Runtime modification support
 */
typedef struct QueueAttributes {
    size_t initial_memory;  // Initial memory allocation
    size_t chunk_size;      // Memory allocation granularity
    size_t warning_limit;   // Memory usage warning threshold
    // Add more attributes as needed
} QueueAttributes;

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
 * Core Operations:
 * - init/destroy: Lifecycle management
 * - find/create: Queue instance handling
 * - enqueue/dequeue: Message operations
 * 
 * Monitoring Operations:
 * - size/memory: Resource tracking
 * - age tracking: Message timing
 * - clear: Emergency cleanup
 */

// System Lifecycle
void queue_system_init();    // Initialize queue system
void queue_system_destroy(); // Clean shutdown of all queues

// Queue Management
Queue* queue_find(const char* name);  // O(1) queue lookup
Queue* queue_create(const char* name, QueueAttributes* attrs);  // Create/get queue
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
