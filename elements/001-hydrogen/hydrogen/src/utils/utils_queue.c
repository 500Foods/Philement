/*
 * Queue Management Implementation
 */
// Global includes 
#include "../hydrogen.h"

// Local includes
#include "utils_queue.h"

// Global queue memory tracking
QueueMemoryMetrics log_queue_memory;
QueueMemoryMetrics webserver_queue_memory;
QueueMemoryMetrics websocket_queue_memory;
QueueMemoryMetrics mdns_server_queue_memory;
QueueMemoryMetrics print_queue_memory;
QueueMemoryMetrics database_queue_memory;
QueueMemoryMetrics mail_relay_queue_memory;
QueueMemoryMetrics notify_queue_memory;

// Initialize queue memory tracking with optional configuration
void init_queue_memory(QueueMemoryMetrics *queue, const AppConfig *config) {
    queue->block_count = 0;
    queue->total_allocation = 0;
    queue->entry_count = 0;
    queue->metrics.virtual_bytes = 0;
    queue->metrics.resident_bytes = 0;
    memset(queue->block_sizes, 0, sizeof(queue->block_sizes));
    
    // Set initial limits and initialization state
    if (config) {
        queue->limits.max_blocks = config->resources.max_queue_blocks;
        queue->limits.block_limit = config->resources.max_queue_blocks;
        queue->limits.early_init = 0;
    } else {
        queue->limits.max_blocks = EARLY_MAX_QUEUE_BLOCKS;
        queue->limits.block_limit = EARLY_BLOCK_LIMIT;
        queue->limits.early_init = 1;
    }
}
// Update queue limits from configuration
void update_queue_limits(QueueMemoryMetrics *queue, const AppConfig *config) {
    if (!config) return;
    
    if (queue->limits.early_init) {
        queue->limits.max_blocks = config->resources.max_queue_blocks;
        queue->limits.block_limit = config->resources.max_queue_blocks;
        queue->limits.early_init = 0;
        
        if (queue->block_count > queue->limits.block_limit) {
            log_this(SR_QUEUES, "Warning: Current queue usage (%zu blocks) exceeds new limit (%zu blocks)", LOG_LEVEL_ALERT, 2, queue->block_count, queue->limits.block_limit);
        }
    } else {
        queue->limits.max_blocks = config->resources.max_queue_blocks;
        queue->limits.block_limit = config->resources.max_queue_blocks;
        
        // Use log_this for normal operation
        if (queue->block_count > queue->limits.block_limit) {
            log_this(SR_QUEUES, "Warning: Current queue usage (%zu blocks) exceeds new limit (%zu blocks)", LOG_LEVEL_ALERT, 2, queue->block_count, queue->limits.block_limit);
        }
    }
}

// Track memory allocation in a queue
void track_queue_allocation(QueueMemoryMetrics *queue, size_t size) {
    if (queue->block_count < queue->limits.block_limit) {
        queue->block_sizes[queue->block_count++] = size;
        queue->total_allocation += size;
        queue->metrics.virtual_bytes = queue->total_allocation;
        queue->metrics.resident_bytes = queue->total_allocation;
    } else {
        log_this(SR_QUEUES, "Queue block limit reached (%zu blocks)", LOG_LEVEL_ALERT, 1, queue->limits.block_limit);
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
