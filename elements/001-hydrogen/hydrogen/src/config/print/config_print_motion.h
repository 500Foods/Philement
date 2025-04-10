/*
 * Print Motion Configuration
 *
 * Defines the configuration structure for printer motion control.
 * This includes settings for acceleration, speed limits, and
 * motion analysis parameters.
 */

#ifndef HYDROGEN_CONFIG_PRINT_MOTION_H
#define HYDROGEN_CONFIG_PRINT_MOTION_H

#include <stddef.h>

// Default acceleration values (mm/s²)
#define DEFAULT_ACCELERATION 3000.0       // Default XY acceleration
#define DEFAULT_Z_ACCELERATION 100.0      // Default Z axis acceleration
#define DEFAULT_E_ACCELERATION 10000.0    // Default extruder acceleration

// Default speed limits (mm/s)
#define DEFAULT_MAX_SPEED_XY 200.0       // Maximum XY movement speed
#define DEFAULT_MAX_SPEED_Z 20.0         // Maximum Z movement speed
#define DEFAULT_MAX_SPEED_TRAVEL 250.0   // Maximum travel speed

// Analysis parameters
#define DEFAULT_Z_VALUES_CHUNK 1000      // Initial chunk size for Z-height analysis

// Motion configuration structure
typedef struct MotionConfig {
    // Acceleration settings (mm/s²)
    double acceleration;          // XY movement acceleration
    double z_acceleration;        // Z axis acceleration
    double e_acceleration;        // Extruder acceleration

    // Speed limits (mm/s)
    double max_speed_xy;         // Maximum XY movement speed
    double max_speed_z;          // Maximum Z axis speed
    double max_speed_travel;     // Maximum travel speed (non-printing moves)
} MotionConfig;

/*
 * Initialize motion configuration with default values
 *
 * @param config Pointer to MotionConfig structure to initialize
 * @return 0 on success, -1 on failure
 */
int config_print_motion_init(MotionConfig* config);

/*
 * Free resources allocated for motion configuration
 *
 * @param config Pointer to MotionConfig structure to cleanup
 */
void config_print_motion_cleanup(MotionConfig* config);

/*
 * Validate motion configuration values
 *
 * Ensures all motion parameters are within safe operating ranges:
 * - Acceleration values are positive and within hardware limits
 * - Speed limits are positive and physically achievable
 * - Values are consistent with each other
 *
 * @param config Pointer to MotionConfig structure to validate
 * @return 0 if valid, -1 if invalid
 */
int config_print_motion_validate(const MotionConfig* config);

#endif /* HYDROGEN_CONFIG_PRINT_MOTION_H */