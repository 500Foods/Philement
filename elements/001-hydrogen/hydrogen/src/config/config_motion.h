/*
 * Motion Configuration
 *
 * Defines the configuration structure and defaults for motion control.
 * This includes settings for acceleration, speed limits, and layer handling.
 */

#ifndef HYDROGEN_CONFIG_MOTION_H
#define HYDROGEN_CONFIG_MOTION_H

#include <stddef.h>

// Project headers
#include "config_forward.h"

// Default values for motion configuration
#define DEFAULT_MAX_LAYERS 1000
#define DEFAULT_ACCELERATION 3000.0
#define DEFAULT_Z_ACCELERATION 100.0
#define DEFAULT_E_ACCELERATION 1000.0
#define DEFAULT_MAX_SPEED_XY 200.0
#define DEFAULT_MAX_SPEED_TRAVEL 300.0
#define DEFAULT_MAX_SPEED_Z 20.0
#define DEFAULT_Z_VALUES_CHUNK 1000

// Motion configuration structure
struct MotionConfig {
    size_t max_layers;         // Maximum number of layers
    double acceleration;       // XY acceleration (mm/s²)
    double z_acceleration;     // Z axis acceleration (mm/s²)
    double e_acceleration;     // Extruder acceleration (mm/s²)
    double max_speed_xy;      // Maximum XY movement speed (mm/s)
    double max_speed_travel;  // Maximum travel speed (mm/s)
    double max_speed_z;       // Maximum Z axis speed (mm/s)
    size_t z_values_chunk;    // Size of Z-value processing chunks
};

/*
 * Initialize motion configuration with default values
 *
 * This function initializes a new MotionConfig structure with default
 * values that provide safe motion parameters.
 *
 * @param config Pointer to MotionConfig structure to initialize
 * @return 0 on success, -1 on failure
 *
 * Error conditions:
 * - If config is NULL
 */
int config_motion_init(MotionConfig* config);

/*
 * Free resources allocated for motion configuration
 *
 * This function cleans up any resources allocated by config_motion_init.
 * Currently, this only zeroes out the structure as there are no dynamically
 * allocated members, but this may change in future versions.
 *
 * @param config Pointer to MotionConfig structure to cleanup
 */
void config_motion_cleanup(MotionConfig* config);

/*
 * Validate motion configuration values
 *
 * This function performs comprehensive validation of the configuration:
 * - Verifies all speeds and accelerations are within safe ranges
 * - Validates layer and chunk size settings
 * - Ensures motion parameters are physically feasible
 *
 * @param config Pointer to MotionConfig structure to validate
 * @return 0 if valid, -1 if invalid
 *
 * Error conditions:
 * - If config is NULL
 * - If any speed or acceleration is negative or too high
 * - If layer or chunk settings are invalid
 */
int config_motion_validate(const MotionConfig* config);

#endif /* HYDROGEN_CONFIG_MOTION_H */