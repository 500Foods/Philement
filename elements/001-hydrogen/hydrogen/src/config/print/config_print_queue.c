/*
 * Print Queue Configuration Implementation
 */

#include <stdlib.h>
#include <string.h>
#include "config_print_queue.h"

int config_print_queue_init(PrintQueueConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize core settings
    config->enabled = DEFAULT_PRINT_QUEUE_ENABLED;
    config->max_queued_jobs = DEFAULT_MAX_QUEUED_JOBS;
    config->max_concurrent_jobs = DEFAULT_MAX_CONCURRENT_JOBS;

    // Initialize subsystems in order of dependency
    if (config_print_priorities_init(&config->priorities) != 0) {
        config_print_queue_cleanup(config);
        return -1;
    }

    if (config_print_timeouts_init(&config->timeouts) != 0) {
        config_print_queue_cleanup(config);
        return -1;
    }

    if (config_print_buffers_init(&config->buffers) != 0) {
        config_print_queue_cleanup(config);
        return -1;
    }

    return 0;
}

void config_print_queue_cleanup(PrintQueueConfig* config) {
    if (!config) {
        return;
    }

    // Cleanup subsystems in reverse order of initialization
    config_print_buffers_cleanup(&config->buffers);
    config_print_timeouts_cleanup(&config->timeouts);
    config_print_priorities_cleanup(&config->priorities);

    // Zero out the structure
    memset(config, 0, sizeof(PrintQueueConfig));
}

static int validate_job_limits(size_t max_queued, size_t max_concurrent) {
    // Validate individual limits
    if (max_queued < MIN_QUEUED_JOBS || max_queued > MAX_QUEUED_JOBS ||
        max_concurrent < MIN_CONCURRENT_JOBS || max_concurrent > MAX_CONCURRENT_JOBS) {
        return -1;
    }

    // Concurrent jobs must be less than queued jobs
    if (max_concurrent > max_queued) {
        return -1;
    }

    return 0;
}

int config_print_queue_validate(const PrintQueueConfig* config) {
    if (!config) {
        return -1;
    }

    // If print queue is enabled, validate all settings
    if (config->enabled) {
        // Validate core settings
        if (validate_job_limits(config->max_queued_jobs,
                              config->max_concurrent_jobs) != 0) {
            return -1;
        }

        // Validate all subsystems
        if (config_print_priorities_validate(&config->priorities) != 0 ||
            config_print_timeouts_validate(&config->timeouts) != 0 ||
            config_print_buffers_validate(&config->buffers) != 0) {
            return -1;
        }

        // Validate cross-subsystem relationships

        // Job processing timeout should be appropriate for buffer sizes
        // Larger buffers need longer processing times
        if (config->timeouts.job_processing_timeout_ms <
            (config->buffers.job_message_size / 1024)) {  // 1ms per KB minimum
            return -1;
        }

        // Status message buffer should be large enough for priority updates
        if (config->buffers.status_message_size < 
            (sizeof(int) * 4)) {  // Space for all priority levels
            return -1;
        }

        // Queue message buffer should handle concurrent jobs
        if (config->buffers.queue_message_size <
            (config->max_concurrent_jobs * sizeof(size_t))) {
            return -1;
        }
    }

    return 0;
}