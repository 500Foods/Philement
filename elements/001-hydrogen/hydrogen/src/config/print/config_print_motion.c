/*
 * Print Motion Configuration Implementation
 */

#include <stdlib.h>
#include "config_print_motion.h"
#include "../../logging/logging.h"

// Validation limits
#define MIN_ACCELERATION 100.0        // Minimum acceleration (mm/s²)
#define MAX_ACCELERATION 15000.0      // Maximum acceleration (mm/s²)
#define MIN_Z_ACCELERATION 10.0       // Minimum Z acceleration
#define MAX_Z_ACCELERATION 1000.0     // Maximum Z acceleration
#define MIN_E_ACCELERATION 100.0      // Minimum extruder acceleration
#define MAX_E_ACCELERATION 20000.0    // Maximum extruder acceleration

#define MIN_SPEED_XY 10.0            // Minimum XY speed (mm/s)
#define MAX_SPEED_XY 500.0           // Maximum XY speed
#define MIN_SPEED_Z 1.0              // Minimum Z speed
#define MAX_SPEED_Z 50.0             // Maximum Z speed
#define MIN_SPEED_TRAVEL 10.0        // Minimum travel speed
#define MAX_SPEED_TRAVEL 500.0       // Maximum travel speed

int config_print_motion_init(MotionConfig* config) {
    if (!config) {
        log_this("PrintMotion", "NULL config pointer in init", LOG_LEVEL_ERROR);
        return -1;
    }

    // Initialize with default values
    config->acceleration = DEFAULT_ACCELERATION;
    config->z_acceleration = DEFAULT_Z_ACCELERATION;
    config->e_acceleration = DEFAULT_E_ACCELERATION;
    config->max_speed_xy = DEFAULT_MAX_SPEED_XY;
    config->max_speed_z = DEFAULT_MAX_SPEED_Z;
    config->max_speed_travel = DEFAULT_MAX_SPEED_TRAVEL;

    return 0;
}

void config_print_motion_cleanup(MotionConfig* config) {
    // No dynamic allocations to clean up
    (void)config;
}

int config_print_motion_validate(const MotionConfig* config) {
    if (!config) {
        log_this("PrintMotion", "NULL config pointer in validate", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate acceleration values
    if (config->acceleration < MIN_ACCELERATION || config->acceleration > MAX_ACCELERATION) {
        log_this("PrintMotion", "XY acceleration out of range", LOG_LEVEL_ERROR);
        return -1;
    }

    if (config->z_acceleration < MIN_Z_ACCELERATION || config->z_acceleration > MAX_Z_ACCELERATION) {
        log_this("PrintMotion", "Z acceleration out of range", LOG_LEVEL_ERROR);
        return -1;
    }

    if (config->e_acceleration < MIN_E_ACCELERATION || config->e_acceleration > MAX_E_ACCELERATION) {
        log_this("PrintMotion", "Extruder acceleration out of range", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate speed limits
    if (config->max_speed_xy < MIN_SPEED_XY || config->max_speed_xy > MAX_SPEED_XY) {
        log_this("PrintMotion", "XY speed out of range", LOG_LEVEL_ERROR);
        return -1;
    }

    if (config->max_speed_z < MIN_SPEED_Z || config->max_speed_z > MAX_SPEED_Z) {
        log_this("PrintMotion", "Z speed out of range", LOG_LEVEL_ERROR);
        return -1;
    }

    if (config->max_speed_travel < MIN_SPEED_TRAVEL || config->max_speed_travel > MAX_SPEED_TRAVEL) {
        log_this("PrintMotion", "Travel speed out of range", LOG_LEVEL_ERROR);
        return -1;
    }

    // Validate speed relationships
    if (config->max_speed_travel < config->max_speed_xy) {
        log_this("PrintMotion", "Travel speed must be >= XY speed", LOG_LEVEL_ERROR);
        return -1;
    }

    return 0;
}