/*
 * Resources Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include "config_resources.h"

int config_resources_init(ResourceConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize memory limits
    config->max_memory_mb = DEFAULT_MAX_MEMORY_MB;
    config->max_buffer_size = DEFAULT_MAX_BUFFER_SIZE;
    config->min_buffer_size = DEFAULT_MIN_BUFFER_SIZE;

    // Initialize queue settings
    config->max_queue_size = DEFAULT_MAX_QUEUE_SIZE;
    config->max_queue_memory_mb = DEFAULT_MAX_QUEUE_MEMORY_MB;
    config->max_queue_blocks = DEFAULT_MAX_QUEUE_BLOCKS;
    config->queue_timeout_ms = DEFAULT_QUEUE_TIMEOUT_MS;

    // Initialize buffer sizes
    config->post_processor_buffer_size = DEFAULT_POST_PROCESSOR_BUFFER_SIZE;

    // Initialize thread limits
    config->min_threads = DEFAULT_MIN_THREADS;
    config->max_threads = DEFAULT_MAX_THREADS;
    config->thread_stack_size = DEFAULT_THREAD_STACK_SIZE;

    // Initialize file limits
    config->max_open_files = DEFAULT_MAX_OPEN_FILES;
    config->max_file_size_mb = DEFAULT_MAX_FILE_SIZE_MB;
    config->max_log_size_mb = DEFAULT_MAX_LOG_SIZE_MB;

    // Initialize monitoring settings
    config->enforce_limits = true;
    config->log_usage = true;
    config->check_interval_ms = 5000;  // 5 seconds default

    return 0;
}

void config_resources_cleanup(ResourceConfig* config) {
    if (!config) {
        return;
    }

    // Zero out the structure
    memset(config, 0, sizeof(ResourceConfig));
}

static int validate_memory_limits(const ResourceConfig* config) {
    // Basic range checks
    if (config->max_memory_mb < 64 || config->max_memory_mb > 16384) {  // 64MB to 16GB
        return -1;
    }

    if (config->max_buffer_size < config->min_buffer_size) {
        return -1;
    }

    if (config->max_buffer_size > (config->max_memory_mb * 1024 * 1024) / 4) {  // Max 1/4 of total memory
        return -1;
    }

    return 0;
}

static int validate_queue_settings(const ResourceConfig* config) {
    // Queue size and memory checks
    if (config->max_queue_size < 10 || config->max_queue_size > 1000000) {
        return -1;
    }

    if (config->max_queue_memory_mb > config->max_memory_mb / 2) {  // Max 1/2 of total memory
        return -1;
    }

    if (config->queue_timeout_ms < 1000 || config->queue_timeout_ms > 300000) {  // 1s to 5min
        return -1;
    }

    if (config->max_queue_blocks < 64 || config->max_queue_blocks > 16384) {  // 64 to 16K blocks
        return -1;
    }

    return 0;
}

static int validate_thread_limits(const ResourceConfig* config) {
    // Thread count and stack size checks
    if (config->min_threads < 1 || config->min_threads > config->max_threads) {
        return -1;
    }

    if (config->max_threads > 256) {  // Reasonable upper limit
        return -1;
    }

    if (config->thread_stack_size < 16384 || config->thread_stack_size > 1048576) {  // 16KB to 1MB
        return -1;
    }

    return 0;
}

static int validate_file_limits(const ResourceConfig* config) {
    // File descriptor and size checks
    if (config->max_open_files < 64 || config->max_open_files > 65535) {
        return -1;
    }

    if (config->max_file_size_mb > config->max_memory_mb * 2) {  // Max 2x memory size
        return -1;
    }

    if (config->max_log_size_mb < 10 || config->max_log_size_mb > 10240) {  // 10MB to 10GB
        return -1;
    }

    return 0;
}

int config_resources_validate(const ResourceConfig* config) {
    if (!config) {
        return -1;
    }

    // Validate each section
    if (validate_memory_limits(config) != 0 ||
        validate_queue_settings(config) != 0 ||
        validate_thread_limits(config) != 0 ||
        validate_file_limits(config) != 0) {
        return -1;
    }

    // Validate monitoring settings
    if (config->check_interval_ms < 1000 || config->check_interval_ms > 60000) {  // 1s to 1min
        return -1;
    }

    return 0;
}