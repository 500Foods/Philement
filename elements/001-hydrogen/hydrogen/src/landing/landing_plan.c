/*
 * Landing Plan System
 * 
 * DESIGN PRINCIPLES:
 * - This file is a lightweight orchestrator only - no subsystem-specific code
 * - All subsystems are equal in importance - no hierarchy
 * - Dependencies determine what's needed, not importance
 * - Processing order is reverse of launch for consistency
 *
 * PLANNING SEQUENCE:
 * 1. Status Assessment:
 *    - Count ready and not-ready subsystems
 *    - Verify at least one subsystem is ready
 *    - Log overall readiness status
 *
 * 2. Dependency Analysis:
 *    - Check each subsystem's dependents
 *    - Ensure dependents are landed or inactive
 *    - Create safe landing sequence
 *
 * 3. Go/No-Go Decision:
 *    - Evaluate readiness of each subsystem
 *    - Verify all dependencies are satisfied
 *    - Make final landing decision
 *
 * Standard Processing Order (reverse of launch):
 * - 15. Print (last launched, first to land)
 * - 14. MailRelay
 * - 13. mDNS Client
 * - 12. mDNS Server
 * - 11. Terminal
 * - 10. WebSocket
 * - 09. Swagger
 * - 08. API
 * - 07. WebServer
 * - 06. Logging
 * - 05. Database
 * - 04. Network
 * - 03. Threads
 * - 02. Payload
 * - 01. Registry (first launched, last to land)
 *
 * Key Points:
 * - Each subsystem's landing must wait for its dependents
 * - Order is reverse of launch to maintain system stability
 * - All decisions are based on actual dependencies, not importance
 */

// System includes
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

// Local includes
#include "landing.h"
#include "landing_plan.h"

// Project includes
#include "../logging/logging.h"
#include "../utils/utils_logging.h"
#include "../registry/registry.h"
#include "../registry/registry_integration.h"

// Forward declarations
static bool check_dependent_states(const char* subsystem, bool* can_land);
static void log_landing_status(const ReadinessResults* results);

/*
 * Execute the landing plan and make Go/No-Go decisions
 * This is the main orchestration function that creates a safe landing sequence
 */
bool handle_landing_plan(const ReadinessResults* results) {
    if (!results) return false;
    
    // Begin LANDING PLAN logging section
    log_this("Landing", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
    log_this("Landing", "LANDING PLAN", LOG_LEVEL_STATE);
    
    // Log overall readiness status
    log_landing_status(results);
    
    if (!results->any_ready) {
        log_this("Landing", "No-Go: No subsystems ready for landing", LOG_LEVEL_ALERT);
        log_this("Landing", "%s", LOG_LEVEL_STATE, LOG_LINE_BREAK);
        return false;
    }
    
    // Define subsystem order (matching landing_readiness.c)
    const char* expected_order[] = {
        "Print", "Mail Relay", "mDNS Client", "mDNS Server", "Terminal",
        "WebSocket", "Swagger", "API", "WebServer", "Database", "Logging",
        "Network", "Payload", "Threads", "Registry"
    };
    
    // Process subsystems in defined order
    for (size_t i = 0; i < sizeof(expected_order) / sizeof(expected_order[0]); i++) {
        const char* subsystem = expected_order[i];
        
        // Find subsystem in results
        bool found = false;
        bool is_ready = false;
        
        for (size_t j = 0; j < results->total_checked; j++) {
            if (strcmp(results->results[j].subsystem, subsystem) == 0) {
                found = true;
                is_ready = results->results[j].ready;
                break;
            }
        }
        
        if (!found) {
            log_this("Landing", "  No-Go: %s", LOG_LEVEL_STATE, subsystem);
            continue;
        }
        
        // Get subsystem ID for dependency checking
        int subsystem_id = get_subsystem_id_by_name(subsystem);
        if (subsystem_id < 0) {
            log_this("Landing", "  No-Go: %s", LOG_LEVEL_STATE, subsystem);
            continue;
        }
        
        // Check if this subsystem can be landed
        bool can_land = true;
        check_dependent_states(subsystem, &can_land);
        
        // Show Go/No-Go status
        if (is_ready && can_land) {
            log_this("Landing", "  Go:    %s", LOG_LEVEL_STATE, subsystem);
        } else {
            log_this("Landing", "  No-Go: %s", LOG_LEVEL_STATE, subsystem);
        }
    }
    
    // Make final landing decision
    log_this("Landing", "LANDING PLAN: Go for landing", LOG_LEVEL_STATE);
    return true;
}

/*
 * Check if all dependents of a subsystem have landed or are inactive
 * Returns true if all dependencies are satisfied
 */
static bool check_dependent_states(const char* subsystem, bool* can_land) {
    for (int j = 0; j < subsystem_registry.count; j++) {
        const SubsystemInfo* dependent = &subsystem_registry.subsystems[j];
        
        // Skip if not a dependent
        bool is_dependent = false;
        for (int k = 0; k < dependent->dependency_count; k++) {
            if (strcmp(dependent->dependencies[k], subsystem) == 0) {
                is_dependent = true;
                break;
            }
        }
        if (!is_dependent) continue;
        
        // Check dependent's state
        SubsystemState state = get_subsystem_state(j);
        if (state != SUBSYSTEM_INACTIVE && state != SUBSYSTEM_ERROR) {
            *can_land = false;
            log_this("Landing", "  %s waiting for dependent %s to land", 
                    LOG_LEVEL_STATE, subsystem, dependent->name);
            return false;
        }
    }
    return true;
}

/*
 * Log the overall landing plan status
 * Provides a summary of subsystem readiness
 */
static void log_landing_status(const ReadinessResults* results) {
    log_this("Landing", "Total Subsystems Checked: %d", LOG_LEVEL_STATE, results->total_checked);
    log_this("Landing", "Ready Subsystems:         %d", LOG_LEVEL_STATE, results->total_ready);
    log_this("Landing", "Not Ready Subsystems:     %d", LOG_LEVEL_STATE, results->total_not_ready);
}