/*
 * Landing System Header
 * 
 * This module defines the interfaces for the landing (shutdown) system.
 * It coordinates the shutdown sequence through specialized modules:
 * - landing_readiness.c - Handles readiness checks
 * - landing_plan.c - Coordinates landing sequence
 * - landing_review.c - Reports landing status
 */

#ifndef LANDING_H
#define LANDING_H

// System includes
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// Project includes
#include "../state/registry/subsystem_registry.h"

// Result of a landing readiness check
typedef struct {
    const char* subsystem;  // Name of the subsystem
    bool ready;             // Is the subsystem ready to land?
    const char** messages;  // Array of readiness messages (NULL-terminated)
} LandingReadiness;

// Structure to track landing status for each subsystem
typedef struct {
    const char* subsystem;    // Subsystem name
    bool ready;              // Ready status from readiness check
    SubsystemState state;    // Current state in registry
    time_t landing_time;     // When landing started
} LandingStatus;

// Structure to hold readiness check results
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

// Core landing functions
bool check_all_landing_readiness(void);

// Memory management
void free_landing_readiness_messages(LandingReadiness* readiness);

#endif /* LANDING_H */