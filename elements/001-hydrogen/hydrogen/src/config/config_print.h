/*
 * Print Configuration
 *
 * Defines the main configuration structure for the print subsystem.
 * This coordinates all print-related configuration components:
 * - Print queue management
 * - Priority settings
 * - Timeout handling
 * - Buffer management
 * - Motion control
 */

#ifndef HYDROGEN_CONFIG_PRINT_H
#define HYDROGEN_CONFIG_PRINT_H

#include <stddef.h>
#include <stdbool.h>
#include <jansson.h>
#include "config_forward.h"
#include "print/config_print_priorities.h"
#include "print/config_print_timeouts.h"
#include "print/config_print_buffers.h"
#include "print/config_print_motion.h"

// Default values
#define DEFAULT_PRINT_ENABLED true
#define DEFAULT_MAX_QUEUED_JOBS 100
#define DEFAULT_MAX_CONCURRENT_JOBS 4

// Validation limits
#define MIN_QUEUED_JOBS 1
#define MAX_QUEUED_JOBS 1000
#define MIN_CONCURRENT_JOBS 1
#define MAX_CONCURRENT_JOBS 16

// Print configuration structure
typedef struct PrintConfig {
    bool enabled;                    // Whether print system is enabled
    size_t max_queued_jobs;         // Maximum number of jobs in queue
    size_t max_concurrent_jobs;      // Maximum concurrent jobs
    
    // Subsystem configurations
    PrintQueuePrioritiesConfig priorities;  // Priority settings
    PrintQueueTimeoutsConfig timeouts;      // Timeout settings
    PrintQueueBuffersConfig buffers;        // Buffer settings
    MotionConfig motion;                    // Motion control settings
} PrintConfig;

/*
 * Load print configuration from JSON
 *
 * This function loads the print configuration from the provided JSON root,
 * applying any environment variable overrides and using secure defaults
 * where values are not specified.
 *
 * @param root JSON root object containing configuration
 * @param config Pointer to AppConfig structure to update
 * @return true if successful, false on error
 */
bool load_print_config(json_t* root, AppConfig* config);

/*
 * Initialize print configuration with default values
 *
 * This function initializes a new PrintConfig structure and all its
 * subsystem configurations with default values.
 *
 * @param config Pointer to PrintConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 * - If any subsystem initialization fails
 */
int config_print_init(PrintConfig* config);

/*
 * Free resources allocated for print configuration
 *
 * This function cleans up the main print configuration and all its
 * subsystem configurations. It safely handles NULL pointers and partial
 * initialization.
 *
 * @param config Pointer to PrintConfig structure to cleanup
 */
void config_print_cleanup(PrintConfig* config);

/*
 * Validate print configuration values
 *
 * This function performs comprehensive validation of all print settings:
 * - Validates core settings (enabled flag, job limits)
 * - Validates all subsystem configurations
 * - Ensures consistency between subsystem settings
 *
 * @param config Pointer to PrintConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If enabled but required settings are missing
 * - If any subsystem validation fails
 * - If settings are inconsistent across subsystems
 */
int config_print_validate(const PrintConfig* config);

#endif /* HYDROGEN_CONFIG_PRINT_H */