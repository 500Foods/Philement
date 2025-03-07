/*
 * Print Queue Timeouts Configuration
 *
 * Defines the configuration structure and defaults for print queue
 * timeout settings. This includes shutdown, job processing, and
 * operation timeouts.
 */

#ifndef HYDROGEN_CONFIG_PRINT_TIMEOUTS_H
#define HYDROGEN_CONFIG_PRINT_TIMEOUTS_H

#include <stddef.h>

// Project headers
#include "config_forward.h"

// Default timeout values (in milliseconds)
#define DEFAULT_SHUTDOWN_WAIT_MS 3000          // 3 seconds
#define DEFAULT_JOB_PROCESSING_TIMEOUT_MS 30000 // 30 seconds
#define DEFAULT_IDLE_TIMEOUT_MS 300000         // 5 minutes
#define DEFAULT_OPERATION_TIMEOUT_MS 5000      // 5 seconds

// Validation limits
#define MIN_SHUTDOWN_WAIT_MS 1000        // 1 second
#define MAX_SHUTDOWN_WAIT_MS 30000       // 30 seconds
#define MIN_JOB_PROCESSING_TIMEOUT_MS 5000    // 5 seconds
#define MAX_JOB_PROCESSING_TIMEOUT_MS 3600000 // 1 hour
#define MIN_IDLE_TIMEOUT_MS 60000       // 1 minute
#define MAX_IDLE_TIMEOUT_MS 3600000     // 1 hour
#define MIN_OPERATION_TIMEOUT_MS 1000    // 1 second
#define MAX_OPERATION_TIMEOUT_MS 60000   // 1 minute

// Print queue timeouts configuration structure
struct PrintQueueTimeoutsConfig {
    size_t shutdown_wait_ms;         // How long to wait during shutdown
    size_t job_processing_timeout_ms; // Maximum time for job processing
    size_t idle_timeout_ms;          // How long to wait when idle
    size_t operation_timeout_ms;      // Timeout for queue operations
};

/*
 * Initialize print queue timeouts configuration with default values
 *
 * This function initializes a new PrintQueueTimeoutsConfig structure
 * with default values that provide reasonable timeout periods.
 *
 * @param config Pointer to PrintQueueTimeoutsConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 */
int config_print_timeouts_init(PrintQueueTimeoutsConfig* config);

/*
 * Free resources allocated for print queue timeouts configuration
 *
 * This function cleans up any resources allocated by config_print_timeouts_init.
 * Currently, this only zeroes out the structure as there are no dynamically
 * allocated members, but this may change in future versions.
 *
 * @param config Pointer to PrintQueueTimeoutsConfig structure to cleanup
 */
void config_print_timeouts_cleanup(PrintQueueTimeoutsConfig* config);

/*
 * Validate print queue timeouts configuration values
 *
 * This function performs comprehensive validation of the configuration:
 * - Verifies all timeout values are within valid ranges
 * - Ensures timeout relationships are logical
 * - Validates operation timing consistency
 *
 * @param config Pointer to PrintQueueTimeoutsConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If any timeout is outside valid range
 * - If timeout relationships are invalid
 */
int config_print_timeouts_validate(const PrintQueueTimeoutsConfig* config);

#endif /* HYDROGEN_CONFIG_PRINT_TIMEOUTS_H */