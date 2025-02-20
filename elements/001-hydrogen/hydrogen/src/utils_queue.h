/*
 * Queue management and memory tracking utilities.
 * 
 * Provides functionality for:
 * - Queue memory tracking
 * - Queue entry tracking
 * - Memory allocation monitoring
 * - Queue metrics collection
 */

#ifndef UTILS_QUEUE_H
#define UTILS_QUEUE_H

#include <stddef.h>

// Constants
#define MAX_QUEUE_BLOCKS 128

// Memory metrics structure
typedef struct {
    size_t virtual_bytes;     // Virtual memory usage in bytes
    size_t resident_bytes;    // Resident memory usage in bytes
} MemoryMetrics;

// Queue memory and entry tracking
typedef struct {
    size_t block_count;                      // Number of allocated blocks
    size_t total_allocation;                 // Total bytes allocated
    size_t entry_count;                      // Number of entries in queue
    MemoryMetrics metrics;                   // Queue memory usage
    size_t block_sizes[MAX_QUEUE_BLOCKS];    // Size of each allocated block
} QueueMemoryMetrics;

// Global queue memory tracking
extern QueueMemoryMetrics log_queue_memory;
extern QueueMemoryMetrics print_queue_memory;

// Queue memory initialization and tracking
void init_queue_memory(QueueMemoryMetrics *queue);
void track_queue_allocation(QueueMemoryMetrics *queue, size_t size);
void track_queue_deallocation(QueueMemoryMetrics *queue, size_t size);

// Queue entry tracking
void track_queue_entry_added(QueueMemoryMetrics *queue);
void track_queue_entry_removed(QueueMemoryMetrics *queue);

#endif // UTILS_QUEUE_H