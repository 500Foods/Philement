/*
 * Print Configuration Implementation
 *
 * Implements configuration loading and management for the print subsystem.
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "config_print.h"

// Load print configuration from JSON
bool load_print_config(json_t* root, AppConfig* config) {
    bool success = true;
    PrintConfig* print_config = &config->print;

    // Zero out the config structure
    memset(print_config, 0, sizeof(PrintConfig));

    // Initialize with secure defaults
    print_config->enabled = true;                // Print system enabled by default
    print_config->max_queued_jobs = 100;         // Default max queued jobs
    print_config->max_concurrent_jobs = 4;       // Default concurrent jobs

    // Initialize priorities with secure defaults
    print_config->priorities.default_priority = 50;      // Normal print jobs
    print_config->priorities.emergency_priority = 100;   // Emergency/critical jobs
    print_config->priorities.maintenance_priority = 75;  // Maintenance operations
    print_config->priorities.system_priority = 90;       // System-level operations

    // Initialize timeouts with secure defaults
    print_config->timeouts.shutdown_wait_ms = 5000;         // 5 second shutdown wait
    print_config->timeouts.job_processing_timeout_ms = 300000;  // 5 minute job timeout

    // Initialize buffers with secure defaults
    print_config->buffers.job_message_size = 4096;      // 4KB job messages
    print_config->buffers.status_message_size = 1024;   // 1KB status messages

    // Initialize motion control with secure defaults
    print_config->motion.max_speed = 100.0;      // Default max speed
    print_config->motion.max_speed_xy = 100.0;   // Default XY speed
    print_config->motion.max_speed_z = 20.0;     // Default Z speed (slower)
    print_config->motion.max_speed_travel = 150.0; // Default travel speed
    print_config->motion.acceleration = 500.0;    // Default acceleration
    print_config->motion.z_acceleration = 100.0;  // Default Z acceleration
    print_config->motion.e_acceleration = 250.0;  // Default extruder acceleration
    print_config->motion.jerk = 10.0;            // Default jerk
    print_config->motion.smooth_moves = true;     // Enable smooth moves by default

    // Process main print section
    success = PROCESS_SECTION(root, "Print");
    success = success && PROCESS_BOOL(root, print_config, enabled, "Print.Enabled", "Print");
    success = success && PROCESS_SIZE(root, print_config, max_queued_jobs, "Print.MaxQueuedJobs", "Print");
    success = success && PROCESS_SIZE(root, print_config, max_concurrent_jobs, "Print.MaxConcurrentJobs", "Print");

    // Process priorities section
    if (success) {
        success = PROCESS_SECTION(root, "Print.Priorities");
        success = success && PROCESS_INT(root, &print_config->priorities, default_priority,
                                       "Print.Priorities.DefaultPriority", "Print");
        success = success && PROCESS_INT(root, &print_config->priorities, emergency_priority,
                                       "Print.Priorities.EmergencyPriority", "Print");
        success = success && PROCESS_INT(root, &print_config->priorities, maintenance_priority,
                                       "Print.Priorities.MaintenancePriority", "Print");
        success = success && PROCESS_INT(root, &print_config->priorities, system_priority,
                                       "Print.Priorities.SystemPriority", "Print");
    }

    // Process timeouts section
    if (success) {
        success = PROCESS_SECTION(root, "Print.Timeouts");
        success = success && PROCESS_SIZE(root, &print_config->timeouts, shutdown_wait_ms,
                                        "Print.Timeouts.ShutdownWaitMs", "Print");
        success = success && PROCESS_SIZE(root, &print_config->timeouts, job_processing_timeout_ms,
                                        "Print.Timeouts.JobProcessingTimeoutMs", "Print");
    }

    // Process buffers section
    if (success) {
        success = PROCESS_SECTION(root, "Print.Buffers");
        success = success && PROCESS_SIZE(root, &print_config->buffers, job_message_size,
                                        "Print.Buffers.JobMessageSize", "Print");
        success = success && PROCESS_SIZE(root, &print_config->buffers, status_message_size,
                                        "Print.Buffers.StatusMessageSize", "Print");
    }

    // Process motion control section
    if (success) {
        success = PROCESS_SECTION(root, "Print.Motion");
        success = success && PROCESS_BOOL(root, &print_config->motion, smooth_moves,
                                        "Print.Motion.SmoothMoves", "Print");
        
        // Process motion control floating point values
        success = success && PROCESS_FLOAT(root, &print_config->motion, max_speed, "Print.Motion.MaxSpeed", "Print");
        success = success && PROCESS_FLOAT(root, &print_config->motion, max_speed_xy, "Print.Motion.MaxSpeedXY", "Print");
        success = success && PROCESS_FLOAT(root, &print_config->motion, max_speed_z, "Print.Motion.MaxSpeedZ", "Print");
        success = success && PROCESS_FLOAT(root, &print_config->motion, max_speed_travel, "Print.Motion.MaxSpeedTravel", "Print");
        success = success && PROCESS_FLOAT(root, &print_config->motion, acceleration, "Print.Motion.Acceleration", "Print");
        success = success && PROCESS_FLOAT(root, &print_config->motion, z_acceleration, "Print.Motion.ZAcceleration", "Print");
        success = success && PROCESS_FLOAT(root, &print_config->motion, e_acceleration, "Print.Motion.EAcceleration", "Print");
        success = success && PROCESS_FLOAT(root, &print_config->motion, jerk, "Print.Motion.Jerk", "Print");
    }

    return success;
}

// Clean up print configuration
void cleanup_print_config(PrintConfig* config) {
    if (!config) return;

    // Zero out the structure
    memset(config, 0, sizeof(PrintConfig));
}

// Helper function for dumping priority settings
void dump_priorities(const PrintQueuePrioritiesConfig* priorities) {
    DUMP_TEXT("――", "Priorities");
    DUMP_INT("――――Default Priority", priorities->default_priority);
    DUMP_INT("――――Emergency Priority", priorities->emergency_priority);
    DUMP_INT("――――Maintenance Priority", priorities->maintenance_priority);
    DUMP_INT("――――System Priority", priorities->system_priority);
}

// Helper function for dumping timeout settings
void dump_timeouts(const PrintQueueTimeoutsConfig* timeouts) {
    DUMP_TEXT("――", "Timeouts");
    DUMP_SIZE("――――Shutdown Wait (ms)", timeouts->shutdown_wait_ms);
    DUMP_SIZE("――――Job Processing Timeout (ms)", timeouts->job_processing_timeout_ms);
}

// Helper function for dumping buffer settings
void dump_buffers(const PrintQueueBuffersConfig* buffers) {
    DUMP_TEXT("――", "Buffers");
    DUMP_SIZE("――――Job Message Size", buffers->job_message_size);
    DUMP_SIZE("――――Status Message Size", buffers->status_message_size);
}

// Helper function for dumping motion control settings with units
void dump_motion(const MotionConfig* motion) {
    DUMP_TEXT("――", "Motion Control");
    char buffer[32];

    snprintf(buffer, sizeof(buffer), "%.2f mm/s", motion->max_speed);
    DUMP_TEXT("――――Max Speed", buffer);
    snprintf(buffer, sizeof(buffer), "%.2f mm/s", motion->max_speed_xy);
    DUMP_TEXT("――――Max XY Speed", buffer);
    snprintf(buffer, sizeof(buffer), "%.2f mm/s", motion->max_speed_z);
    DUMP_TEXT("――――Max Z Speed", buffer);
    snprintf(buffer, sizeof(buffer), "%.2f mm/s", motion->max_speed_travel);
    DUMP_TEXT("――――Max Travel Speed", buffer);
    snprintf(buffer, sizeof(buffer), "%.2f mm/s²", motion->acceleration);
    DUMP_TEXT("――――Acceleration", buffer);
    snprintf(buffer, sizeof(buffer), "%.2f mm/s²", motion->z_acceleration);
    DUMP_TEXT("――――Z Acceleration", buffer);
    snprintf(buffer, sizeof(buffer), "%.2f mm/s²", motion->e_acceleration);
    DUMP_TEXT("――――Extruder Acceleration", buffer);
    snprintf(buffer, sizeof(buffer), "%.2f mm/s³", motion->jerk);
    DUMP_TEXT("――――Jerk", buffer);
    DUMP_BOOL("――――Smooth Moves", motion->smooth_moves);
}

// Dump print configuration for debugging
void dump_print_config(const PrintConfig* config) {
    if (!config) {
        DUMP_TEXT("", "Cannot dump NULL print config");
        return;
    }

    // Dump main settings
    DUMP_BOOL("Enabled", config->enabled);
    DUMP_SIZE("Max Queued Jobs", config->max_queued_jobs);
    DUMP_SIZE("Max Concurrent Jobs", config->max_concurrent_jobs);

    // Dump subsystem configurations
    dump_priorities(&config->priorities);
    dump_timeouts(&config->timeouts);
    dump_buffers(&config->buffers);
    dump_motion(&config->motion);
}
