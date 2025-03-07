/*
 * Print Queue Timeouts Configuration Implementation
 */

#include <string.h>
#include "config_print_timeouts.h"

int config_print_timeouts_init(PrintQueueTimeoutsConfig* config) {
    if (!config) {
        return -1;
    }

    // Initialize with default timeout values
    config->shutdown_wait_ms = DEFAULT_SHUTDOWN_WAIT_MS;
    config->job_processing_timeout_ms = DEFAULT_JOB_PROCESSING_TIMEOUT_MS;
    config->idle_timeout_ms = DEFAULT_IDLE_TIMEOUT_MS;
    config->operation_timeout_ms = DEFAULT_OPERATION_TIMEOUT_MS;

    return 0;
}

void config_print_timeouts_cleanup(PrintQueueTimeoutsConfig* config) {
    if (!config) {
        return;
    }

    // Zero out the structure
    memset(config, 0, sizeof(PrintQueueTimeoutsConfig));
}

static int validate_timeout_range(size_t value, size_t min, size_t max) {
    return value >= min && value <= max;
}

int config_print_timeouts_validate(const PrintQueueTimeoutsConfig* config) {
    if (!config) {
        return -1;
    }

    // Validate individual timeout ranges
    if (!validate_timeout_range(config->shutdown_wait_ms,
                              MIN_SHUTDOWN_WAIT_MS,
                              MAX_SHUTDOWN_WAIT_MS) ||
        !validate_timeout_range(config->job_processing_timeout_ms,
                              MIN_JOB_PROCESSING_TIMEOUT_MS,
                              MAX_JOB_PROCESSING_TIMEOUT_MS) ||
        !validate_timeout_range(config->idle_timeout_ms,
                              MIN_IDLE_TIMEOUT_MS,
                              MAX_IDLE_TIMEOUT_MS) ||
        !validate_timeout_range(config->operation_timeout_ms,
                              MIN_OPERATION_TIMEOUT_MS,
                              MAX_OPERATION_TIMEOUT_MS)) {
        return -1;
    }

    // Validate timeout relationships
    // Operation timeout should be less than job processing timeout
    if (config->operation_timeout_ms >= config->job_processing_timeout_ms) {
        return -1;
    }

    // Shutdown wait should be less than idle timeout
    if (config->shutdown_wait_ms >= config->idle_timeout_ms) {
        return -1;
    }

    // Operation timeout should be less than shutdown wait
    if (config->operation_timeout_ms >= config->shutdown_wait_ms) {
        return -1;
    }

    return 0;
}