/*
 * Print Queue Buffers Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include "config_print_buffers.h"

// Maximum total buffer memory (2MB)
#define MAX_TOTAL_BUFFER_MEMORY (2 * 1024 * 1024)

int config_print_buffers_init(PrintQueueBuffersConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize message sizes
    config->job_message_size = DEFAULT_JOB_MESSAGE_SIZE;
    config->status_message_size = DEFAULT_STATUS_MESSAGE_SIZE;
    config->queue_message_size = DEFAULT_QUEUE_MESSAGE_SIZE;

    // Initialize operation buffers
    config->command_buffer_size = DEFAULT_COMMAND_BUFFER_SIZE;
    config->response_buffer_size = DEFAULT_RESPONSE_BUFFER_SIZE;

    return 0;
}

void config_print_buffers_cleanup(PrintQueueBuffersConfig* config) {
    if (!config) {
        return;
    }

    // Zero out the structure
    memset(config, 0, sizeof(PrintQueueBuffersConfig));
}

static int validate_message_size(size_t size) {
    return size >= MIN_MESSAGE_SIZE && size <= MAX_MESSAGE_SIZE;
}

static int validate_buffer_size(size_t size) {
    return size >= MIN_BUFFER_SIZE && size <= MAX_BUFFER_SIZE;
}

static size_t calculate_total_buffer_memory(const PrintQueueBuffersConfig* config) {
    return config->job_message_size +
           config->status_message_size +
           config->queue_message_size +
           config->command_buffer_size +
           config->response_buffer_size;
}

int config_print_buffers_validate(const PrintQueueBuffersConfig* config) {
    if (!config) {
        return -1;
    }

    // Validate message sizes
    if (!validate_message_size(config->job_message_size) ||
        !validate_message_size(config->status_message_size) ||
        !validate_message_size(config->queue_message_size)) {
        return -1;
    }

    // Validate operation buffer sizes
    if (!validate_buffer_size(config->command_buffer_size) ||
        !validate_buffer_size(config->response_buffer_size)) {
        return -1;
    }

    // Validate buffer size relationships
    
    // Response buffer should be larger than command buffer
    // to accommodate command output and status information
    if (config->response_buffer_size < config->command_buffer_size) {
        return -1;
    }

    // Status message size should be smaller than job message size
    // as status updates are typically smaller than job data
    if (config->status_message_size >= config->job_message_size) {
        return -1;
    }

    // Queue message size should be between status and job message sizes
    // to handle queue operations efficiently
    if (config->queue_message_size <= config->status_message_size ||
        config->queue_message_size >= config->job_message_size) {
        return -1;
    }

    // Validate total memory usage
    if (calculate_total_buffer_memory(config) > MAX_TOTAL_BUFFER_MEMORY) {
        return -1;
    }

    return 0;
}