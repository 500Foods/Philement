/*
 * Landing Database Subsystem
 * 
 * This module handles the landing (shutdown) sequence for the database subsystem.
 * Note: Database is not a standalone service, but rather a configuration handler.
 * It provides functions for:
 * - Checking database landing readiness
 * - Managing database configuration cleanup
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "landing.h"
#include "landing_readiness.h"
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../registry/registry.h"
#include "../registry/registry_integration.h"

// Check if the database subsystem is ready to land
LandingReadiness check_database_landing_readiness(void) {
    LandingReadiness readiness = {0};
    readiness.subsystem = "Database";
    
    // Allocate space for messages (including NULL terminator)
    readiness.messages = malloc(4 * sizeof(char*));
    if (!readiness.messages) {
        readiness.ready = false;
        return readiness;
    }
    
    // Add initial subsystem identifier
    readiness.messages[0] = strdup("Database");
    
    // Check if database is actually running
    if (!is_subsystem_running_by_name("Database")) {
        readiness.ready = false;
        readiness.messages[1] = strdup("  No-Go:   Database not running");
        readiness.messages[2] = strdup("  Decide:  No-Go For Landing of Database");
        readiness.messages[3] = NULL;
        return readiness;
    }
    
    // Database is always ready for landing as it's just a configuration handler
    readiness.ready = true;
    readiness.messages[1] = strdup("  Go:      No active database operations");
    readiness.messages[2] = strdup("  Decide:  Go For Landing of Database");
    readiness.messages[3] = NULL;
    
    return readiness;
}

// Use shutdown function from launch-database.c
extern void shutdown_database(void);