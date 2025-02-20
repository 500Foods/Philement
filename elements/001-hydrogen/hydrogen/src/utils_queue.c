// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Project headers
#include "utils_queue.h"
#include "logging.h"

// System headers
#include <string.h>

// Global queue memory tracking
QueueMemoryMetrics log_queue_memory;
QueueMemoryMetrics print_queue_memory;

// Initialize queue memory tracking
void init_queue_memory(QueueMemoryMetrics *queue) {
    queue->block_count = 0;
    queue->total_allocation = 0;
    queue->entry_count = 0;
    queue->metrics.virtual_bytes = 0;
    queue->metrics.resident_bytes = 0;
    memset(queue->block_sizes, 0, sizeof(size_t) * MAX_QUEUE_BLOCKS);
}

// Track memory allocation in a queue
void track_queue_allocation(QueueMemoryMetrics *queue, size_t size) {
    if (queue->block_count < MAX_QUEUE_BLOCKS) {
        queue->block_sizes[queue->block_count++] = size;
        queue->total_allocation += size;
        queue->metrics.virtual_bytes = queue->total_allocation;
        queue->metrics.resident_bytes = queue->total_allocation;
    }
}

// Track memory deallocation in a queue
void track_queue_deallocation(QueueMemoryMetrics *queue, size_t size) {
    for (size_t i = 0; i < queue->block_count; i++) {
        if (queue->block_sizes[i] == size) {
            // Remove this block by shifting others down
            for (size_t j = i; j < queue->block_count - 1; j++) {
                queue->block_sizes[j] = queue->block_sizes[j + 1];
            }
            queue->block_count--;
            queue->total_allocation -= size;
            queue->metrics.virtual_bytes = queue->total_allocation;
            queue->metrics.resident_bytes = queue->total_allocation;
            break;
        }
    }
}

// Track when an entry is added to a queue
void track_queue_entry_added(QueueMemoryMetrics *queue) {
    queue->entry_count++;
}

// Track when an entry is removed from a queue
void track_queue_entry_removed(QueueMemoryMetrics *queue) {
    if (queue->entry_count > 0) {
        queue->entry_count--;
    }
}