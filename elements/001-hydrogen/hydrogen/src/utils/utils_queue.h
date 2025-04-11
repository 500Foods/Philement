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

#include "../config/config_forward.h"  // For AppConfig forward declaration
#include "../config/types/config_queue_constants.h"  // Queue-related constants

// Early initialization limits - subset of full configuration
#define EARLY_MAX_QUEUE_BLOCKS MIN_QUEUE_BLOCKS  // Use minimum from config
#define EARLY_BLOCK_LIMIT (MIN_QUEUE_BLOCKS / 2) // Half of minimum blocks

// Memory metrics structure
typedef struct {
    size_t max_blocks;        // Runtime limit from configuration
    size_t block_limit;       // Current block limit (from config)
    int early_init;          // Flag to indicate early initialization
} QueueLimits;
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
    QueueLimits limits;                      // Runtime limits from configuration
} QueueMemoryMetrics;

// Global queue memory tracking
extern QueueMemoryMetrics log_queue_memory;
extern QueueMemoryMetrics print_queue_memory;

// Queue memory initialization and tracking
void init_queue_memory(QueueMemoryMetrics *queue, const AppConfig *config);
void update_queue_limits(QueueMemoryMetrics *queue, const AppConfig *config);
void track_queue_allocation(QueueMemoryMetrics *queue, size_t size);
void track_queue_deallocation(QueueMemoryMetrics *queue, size_t size);

// Queue entry tracking
void track_queue_entry_added(QueueMemoryMetrics *queue);
void track_queue_entry_removed(QueueMemoryMetrics *queue);

// Update all queue limits from configuration
void update_queue_limits_from_config(const AppConfig *config);

#endif // UTILS_QUEUE_H