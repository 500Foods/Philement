/*
 * Print Priorities Configuration
 *
 * Defines the configuration structure and defaults for print job priorities.
 * This includes settings for different job types and their relative priorities.
 */

#ifndef HYDROGEN_CONFIG_PRINT_PRIORITIES_H
#define HYDROGEN_CONFIG_PRINT_PRIORITIES_H

#include <src/globals.h>
#include <stdbool.h>
#include <jansson.h>

// Print priorities configuration structure
typedef struct PrintPrioritiesConfig {
    int default_priority;      // Default priority for normal jobs
    int emergency_priority;    // Priority for emergency jobs
    int maintenance_priority;  // Priority for maintenance jobs
    int system_priority;       // Priority for system jobs
} PrintPrioritiesConfig;

/*
 * Initialize print priorities configuration with defaults
 *
 * This function initializes a new PrintPrioritiesConfig structure with
 * default values that provide a reasonable baseline configuration.
 *
 * @param config Pointer to PrintPrioritiesConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 */
int config_print_priorities_init(PrintPrioritiesConfig* config);

/*
 * Free resources allocated for print priorities configuration
 *
 * This function cleans up any resources allocated by config_print_priorities_init.
 * Currently, this only zeroes out the structure as there are no dynamically
 * allocated resources.
 *
 * @param config Pointer to PrintPrioritiesConfig structure to cleanup
 */
void config_print_priorities_cleanup(PrintPrioritiesConfig* config);

/*
 * Validate print priorities configuration values
 *
 * This function performs comprehensive validation of the configuration:
 * - Verifies all priorities are within valid range
 * - Ensures emergency priority is lowest (highest priority)
 * - Validates relative priority ordering
 *
 * @param config Pointer to PrintPrioritiesConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If any priority is out of valid range
 * - If priorities violate required ordering
 */
int config_print_priorities_validate(const PrintPrioritiesConfig* config);

#endif /* HYDROGEN_CONFIG_PRINT_PRIORITIES_H */
