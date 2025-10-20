/*
 * Print Configuration
 *
 * Defines the configuration structure for the print subsystem.
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

// Print queue priorities configuration
typedef struct PrintQueuePrioritiesConfig {
    int default_priority;     // Priority for normal print jobs
    int emergency_priority;   // Priority for emergency jobs
    int maintenance_priority; // Priority for maintenance tasks
    int system_priority;      // Priority for system operations
} PrintQueuePrioritiesConfig;

// Print queue timeouts configuration
typedef struct PrintQueueTimeoutsConfig {
    size_t shutdown_wait_ms;          // Time to wait for queue drain on shutdown
    size_t job_processing_timeout_ms;  // Maximum time for job processing
} PrintQueueTimeoutsConfig;

// Print queue buffer configuration
typedef struct PrintQueueBuffersConfig {
    size_t job_message_size;     // Maximum size of job messages
    size_t status_message_size;  // Maximum size of status messages
} PrintQueueBuffersConfig;

// Motion control configuration
typedef struct MotionConfig {
    double max_speed;         // Maximum movement speed
    double max_speed_xy;      // Maximum XY movement speed
    double max_speed_z;       // Maximum Z movement speed
    double max_speed_travel;  // Maximum travel speed
    double acceleration;      // Movement acceleration
    double z_acceleration;    // Z axis acceleration
    double e_acceleration;    // Extruder acceleration
    double jerk;             // Maximum jerk (rate of acceleration change)
    bool smooth_moves;        // Whether to use acceleration smoothing
} MotionConfig;

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
 * Helper function for dumping priority settings
 *
 * @param priorities Pointer to PrintQueuePrioritiesConfig structure to dump
 */
void dump_priorities(const PrintQueuePrioritiesConfig* priorities);

/*
 * Helper function for dumping timeout settings
 *
 * @param timeouts Pointer to PrintQueueTimeoutsConfig structure to dump
 */
void dump_timeouts(const PrintQueueTimeoutsConfig* timeouts);

/*
 * Helper function for dumping buffer settings
 *
 * @param buffers Pointer to PrintQueueBuffersConfig structure to dump
 */
void dump_buffers(const PrintQueueBuffersConfig* buffers);

/*
 * Helper function for dumping motion control settings with units
 *
 * @param motion Pointer to MotionConfig structure to dump
 */
void dump_motion(const MotionConfig* motion);

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
 * Clean up print configuration
 *
 * This function cleans up the main print configuration and all its
 * subsystem configurations. It safely handles NULL pointers and partial
 * initialization.
 *
 * @param config Pointer to PrintConfig structure to cleanup
 */
void cleanup_print_config(PrintConfig* config);

/*
 * Dump print configuration for debugging
 *
 * This function outputs the current state of the print configuration
 * and all its subsystems in a structured format.
 *
 * @param config Pointer to PrintConfig structure to dump
 */
void dump_print_config(const PrintConfig* config);

#endif /* HYDROGEN_CONFIG_PRINT_H */
