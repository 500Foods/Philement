/*
 * System Resources Configuration Implementation
 *
 * This module handles the configuration for system-wide resource limits,
 * including queue sizes, buffer sizes, and memory allocations.
 *
 * Design Decisions:
 * - Conservative default values to ensure stability
 * - Memory-conscious buffer sizes for embedded systems
 * - Queue sizes optimized for typical workloads
 */

#define _GNU_SOURCE  // For strdup

// Core system headers
#include <sys/types.h>

// Standard C headers
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Project headers
#include "config_resources.h"
#include "../logging/logging.h"

int config_resources_init(ResourceConfig* config) {
    if (!config) {
        log_this("Resources", "Null config pointer provided", LOG_LEVEL_ERROR);
        return -1;
    }

    // Initialize queue-related settings
    config->max_queue_blocks = DEFAULT_MAX_QUEUE_BLOCKS;
    config->queue_hash_size = DEFAULT_QUEUE_HASH_SIZE;
    config->default_capacity = DEFAULT_QUEUE_CAPACITY;

    // Initialize buffer sizes
    config->message_buffer_size = DEFAULT_MESSAGE_BUFFER_SIZE;
    config->max_log_message_size = DEFAULT_MAX_LOG_MESSAGE_SIZE;
    config->line_buffer_size = DEFAULT_LINE_BUFFER_SIZE;
    config->post_processor_buffer_size = DEFAULT_POST_PROCESSOR_BUFFER_SIZE;
    config->log_buffer_size = DEFAULT_LOG_BUFFER_SIZE;
    config->json_message_size = DEFAULT_JSON_MESSAGE_SIZE;
    config->log_entry_size = DEFAULT_LOG_ENTRY_SIZE;

    return 0;
}

void config_resources_cleanup(ResourceConfig* config) {
    if (!config) {
        return;
    }

    // Zero out the structure
    // No dynamic memory to free, but clear for safety
    memset(config, 0, sizeof(ResourceConfig));
}

int config_resources_validate(const ResourceConfig* config) {
    if (!config) {
        log_this("Resources", "Null config pointer in validation", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate queue settings
    if (config->max_queue_blocks == 0 || 
        config->queue_hash_size == 0 || 
        config->default_capacity == 0) {
        log_this("Resources", "Invalid queue settings", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate buffer sizes
    if (config->message_buffer_size == 0 || 
        config->max_log_message_size == 0 || 
        config->line_buffer_size == 0 || 
        config->post_processor_buffer_size == 0 || 
        config->log_buffer_size == 0 || 
        config->json_message_size == 0 || 
        config->log_entry_size == 0) {
        log_this("Resources", "Invalid buffer size settings", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate relationships between buffer sizes
    if (config->log_entry_size > config->log_buffer_size) {
        log_this("Resources", "Log entry size exceeds buffer size", LOG_LEVEL_ERROR);
        return -1;
    }

    if (config->message_buffer_size < config->max_log_message_size) {
        log_this("Resources", "Message buffer smaller than max log message", LOG_LEVEL_ERROR);
        return -1;
    }

    return 0;
}