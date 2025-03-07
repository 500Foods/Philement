/*
 * System Monitoring Configuration
 *
 * Defines the configuration structure and defaults for system monitoring.
 * This includes settings for resource monitoring, metrics collection,
 * and warning thresholds.
 */

#ifndef HYDROGEN_CONFIG_MONITORING_H
#define HYDROGEN_CONFIG_MONITORING_H

#include <stddef.h>

// Project headers
#include "config_forward.h"

// Default monitoring intervals (in milliseconds)
#define DEFAULT_STATUS_UPDATE_MS 1000     // 1 second
#define DEFAULT_RESOURCE_CHECK_MS 5000    // 5 seconds
#define DEFAULT_METRICS_UPDATE_MS 1000    // 1 second

// Default warning thresholds
#define DEFAULT_MEMORY_WARNING_PERCENT 90  // 90% memory usage
#define DEFAULT_DISK_WARNING_PERCENT 90    // 90% disk usage
#define DEFAULT_LOAD_WARNING 5.0          // 5.0 load average

// Validation limits
#define MIN_UPDATE_INTERVAL_MS 100        // 100ms minimum interval
#define MAX_UPDATE_INTERVAL_MS 60000      // 60 seconds maximum interval
#define MIN_WARNING_PERCENT 1             // 1% minimum threshold
#define MAX_WARNING_PERCENT 99            // 99% maximum threshold
#define MIN_LOAD_WARNING 0.1              // 0.1 minimum load threshold
#define MAX_LOAD_WARNING 100.0            // 100.0 maximum load threshold

// Monitoring configuration structure
struct MonitoringConfig {
    // Update intervals
    size_t status_update_ms;     // How often to update system status
    size_t resource_check_ms;    // How often to check resource usage
    size_t metrics_update_ms;    // How often to update metrics
    
    // Warning thresholds
    int memory_warning_percent;  // Memory usage warning threshold
    int disk_warning_percent;    // Disk usage warning threshold
    double load_warning;         // System load warning threshold
};

/*
 * Initialize monitoring configuration with default values
 *
 * This function initializes a new MonitoringConfig structure with default
 * values that provide reasonable monitoring intervals and thresholds.
 *
 * @param config Pointer to MonitoringConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 */
int config_monitoring_init(MonitoringConfig* config);

/*
 * Free resources allocated for monitoring configuration
 *
 * This function cleans up any resources allocated by config_monitoring_init.
 * Currently, this only zeroes out the structure as there are no dynamically
 * allocated members, but this may change in future versions.
 *
 * @param config Pointer to MonitoringConfig structure to cleanup
 */
void config_monitoring_cleanup(MonitoringConfig* config);

/*
 * Validate monitoring configuration values
 *
 * This function performs comprehensive validation of the configuration:
 * - Verifies all intervals are within acceptable ranges
 * - Validates warning thresholds
 * - Ensures monitoring settings are consistent
 *
 * @param config Pointer to MonitoringConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If any interval is outside valid range
 * - If any threshold is invalid
 * - If intervals create timing conflicts
 */
int config_monitoring_validate(const MonitoringConfig* config);

#endif /* HYDROGEN_CONFIG_MONITORING_H */