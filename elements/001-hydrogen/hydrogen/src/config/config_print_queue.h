/*
 * Print Queue Configuration
 *
 * Defines the main configuration structure for the print queue subsystem.
 * This coordinates all print queue-related configuration components.
 */

#ifndef HYDROGEN_CONFIG_PRINT_QUEUE_H
#define HYDROGEN_CONFIG_PRINT_QUEUE_H

// Project headers
#include "config_forward.h"
#include "config_print_priorities.h"
#include "config_print_timeouts.h"
#include "config_print_buffers.h"

// Default values
#define DEFAULT_PRINT_QUEUE_ENABLED 1
#define DEFAULT_MAX_QUEUED_JOBS 100
#define DEFAULT_MAX_CONCURRENT_JOBS 4

// Validation limits
#define MIN_QUEUED_JOBS 1
#define MAX_QUEUED_JOBS 1000
#define MIN_CONCURRENT_JOBS 1
#define MAX_CONCURRENT_JOBS 16

// Print queue configuration structure
struct PrintQueueConfig {
    int enabled;                     // Whether print queue is enabled
    size_t max_queued_jobs;         // Maximum number of jobs in queue
    size_t max_concurrent_jobs;      // Maximum concurrent jobs
    
    // Subsystem configurations
    PrintQueuePrioritiesConfig priorities;  // Priority settings
    PrintQueueTimeoutsConfig timeouts;      // Timeout settings
    PrintQueueBuffersConfig buffers;        // Buffer settings
};

/*
 * Initialize print queue configuration with default values
 *
 * This function initializes a new PrintQueueConfig structure and all its
 * subsystem configurations with default values.
 *
 * @param config Pointer to PrintQueueConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 * - If any subsystem initialization fails
 */
int config_print_queue_init(PrintQueueConfig* config);

/*
 * Free resources allocated for print queue configuration
 *
 * This function cleans up the main print queue configuration and all its
 * subsystem configurations. It safely handles NULL pointers and partial
 * initialization.
 *
 * @param config Pointer to PrintQueueConfig structure to cleanup
 */
void config_print_queue_cleanup(PrintQueueConfig* config);

/*
 * Validate print queue configuration values
 *
 * This function performs comprehensive validation of all print queue settings:
 * - Validates core queue settings (enabled flag, job limits)
 * - Validates all subsystem configurations
 * - Ensures consistency between subsystem settings
 *
 * @param config Pointer to PrintQueueConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If enabled but required settings are missing
 * - If any subsystem validation fails
 * - If settings are inconsistent across subsystems
 */
int config_print_queue_validate(const PrintQueueConfig* config);

#endif /* HYDROGEN_CONFIG_PRINT_QUEUE_H */