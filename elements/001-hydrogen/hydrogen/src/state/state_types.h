/*
 * Shared State Types
 *
 * This module defines common types used across the state management system.
 * These types are shared between the launch system, landing system, and 
 * subsystem registry to avoid circular dependencies.
 */

#ifndef STATE_TYPES_H
#define STATE_TYPES_H

// System includes
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// Subsystem state enum
typedef enum {
    SUBSYSTEM_INACTIVE,  // Not started
    SUBSYSTEM_STARTING,  // In the process of starting
    SUBSYSTEM_RUNNING,   // Running normally
    SUBSYSTEM_STOPPING,  // In the process of stopping
    SUBSYSTEM_ERROR      // Error state
} SubsystemState;

// Result of a readiness check (used by both launch and landing)
typedef struct {
    const char* subsystem;  // Name of the subsystem
    bool ready;             // Is the subsystem ready?
    const char** messages;  // Array of readiness messages (NULL-terminated)
} LaunchReadiness;

// Structure to hold readiness check results (used by both launch and landing)
typedef struct {
    struct {
        const char* subsystem;
        bool ready;
    } results[15];  // Must match number of subsystems
    size_t total_checked;
    size_t total_ready;
    size_t total_not_ready;
    bool any_ready;
} ReadinessResults;

// Structure to track status for each subsystem (used by both launch and landing)
typedef struct {
    const char* subsystem;    // Subsystem name
    bool ready;              // Ready status from readiness check
    SubsystemState state;    // Current state in registry
    time_t state_time;       // When state last changed (launch_time or landing_time)
} SubsystemStatus;

// Memory management
void free_readiness_messages(LaunchReadiness* readiness);

#endif /* STATE_TYPES_H */