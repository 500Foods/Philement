/*
 * Print Queue Priorities Configuration
 *
 * Defines the configuration structure and defaults for print queue
 * priority levels. This includes settings for different job types
 * and their relative priorities.
 */

#ifndef HYDROGEN_CONFIG_PRINT_PRIORITIES_H
#define HYDROGEN_CONFIG_PRINT_PRIORITIES_H

// Project headers
#include "config_forward.h"

// Default priority values (higher number = higher priority)
#define DEFAULT_PRIORITY 50            // Normal print jobs
#define DEFAULT_EMERGENCY_PRIORITY 100 // Emergency/critical jobs
#define DEFAULT_MAINTENANCE_PRIORITY 75 // Maintenance operations
#define DEFAULT_SYSTEM_PRIORITY 90     // System-level operations

// Validation limits
#define MIN_PRIORITY 1
#define MAX_PRIORITY 100
#define MIN_PRIORITY_SPREAD 10  // Minimum difference between priority levels

// Print queue priorities configuration structure
struct PrintQueuePrioritiesConfig {
    int default_priority;     // Priority for normal print jobs
    int emergency_priority;   // Priority for emergency jobs
    int maintenance_priority; // Priority for maintenance tasks
    int system_priority;      // Priority for system operations
};

/*
 * Initialize print queue priorities configuration with default values
 *
 * This function initializes a new PrintQueuePrioritiesConfig structure
 * with default values that provide a reasonable priority hierarchy.
 *
 * @param config Pointer to PrintQueuePrioritiesConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 */
int config_print_priorities_init(PrintQueuePrioritiesConfig* config);

/*
 * Free resources allocated for print queue priorities configuration
 *
 * This function cleans up any resources allocated by config_print_priorities_init.
 * Currently, this only zeroes out the structure as there are no dynamically
 * allocated members, but this may change in future versions.
 *
 * @param config Pointer to PrintQueuePrioritiesConfig structure to cleanup
 */
void config_print_priorities_cleanup(PrintQueuePrioritiesConfig* config);

/*
 * Validate print queue priorities configuration values
 *
 * This function performs comprehensive validation of the configuration:
 * - Verifies all priority values are within valid ranges
 * - Ensures proper hierarchy between different priority levels
 * - Validates priority spreads to prevent conflicts
 *
 * @param config Pointer to PrintQueuePrioritiesConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If any priority is outside valid range
 * - If priority hierarchy is invalid
 * - If priority spreads are insufficient
 */
int config_print_priorities_validate(const PrintQueuePrioritiesConfig* config);

#endif /* HYDROGEN_CONFIG_PRINT_PRIORITIES_H */