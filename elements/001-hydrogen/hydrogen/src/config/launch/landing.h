/*
 * Landing Readiness System
 * 
 * This module coordinates pre-landing checks for all subsystems.
 * It ensures that resources can be safely freed and reports readiness status.
 * It is the counterpart to the Launch Readiness system but for shutdown.
 */

#ifndef HYDROGEN_LANDING_H
#define HYDROGEN_LANDING_H

#include <stdbool.h>

// Structure to hold landing readiness check results
typedef struct {
    const char* subsystem;  // Name of the subsystem
    bool ready;             // Whether the subsystem is ready to land (shutdown)
    char** messages;        // Array of messages (NULL-terminated)
} LandingReadiness;

// Check if all subsystems are ready to land (shutdown)
// Returns true if at least one subsystem is ready to land
bool check_all_landing_readiness(void);

// Free the messages in a LandingReadiness struct
void free_landing_readiness_messages(LandingReadiness* readiness);

#endif // HYDROGEN_LANDING_H