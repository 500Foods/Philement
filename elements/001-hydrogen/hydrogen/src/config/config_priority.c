/*
 * Priority level management for configuration system
 * 
 * Why This Architecture:
 * 1. Centralization
 *    - Single source of truth for priority definitions
 *    - Consistent formatting across all log outputs
 *    - Unified priority management
 * 
 * 2. Performance
 *    - Pre-calculated widths avoid runtime formatting
 *    - Single calculation during initialization
 *    - Minimal memory footprint
 * 
 * 3. Maintainability
 *    - Clear separation of priority management
 *    - Easy to extend or modify levels
 *    - Consistent formatting rules
 * 
 * Thread Safety:
 * - Initialization is not thread-safe
 * - Should only be called during system startup
 * - Read-only access after initialization
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Standard C headers
#include <string.h>

// Project headers
#include "config_priority.h"
#include "../logging/logging.h"

// Global variables
int MAX_PRIORITY_LABEL_WIDTH = 5;    // All log level names are 5 characters
int MAX_SUBSYSTEM_LABEL_WIDTH = 18;  // Default minimum width

// Priority level definitions
const PriorityLevel DEFAULT_PRIORITY_LEVELS[NUM_PRIORITY_LEVELS] = {
    {0, "TRACE"},
    {1, "DEBUG"},
    {2, "STATE"},
    {3, "ALERT"},
    {4, "ERROR"},
    {5, "FATAL"},
    {6, "QUIET"}
};

/*
 * Calculate maximum width of priority labels
 * 
 * Why pre-calculate?
 * 1. Performance
 *    - Avoids repeated calculations
 *    - Consistent formatting overhead
 *    - Single calculation at startup
 * 
 * 2. Consistency
 *    - Ensures uniform log formatting
 *    - Maintains log readability
 *    - Supports dynamic priority systems
 * 
 * 3. Maintainability
 *    - Clear formatting rules
 *    - Easy to update or extend
 *    - Centralized width management
 */
void calculate_max_priority_label_width(void) {
    MAX_PRIORITY_LABEL_WIDTH = 0;
    
    for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        int label_width = strlen(DEFAULT_PRIORITY_LEVELS[i].label);
        if (label_width > MAX_PRIORITY_LABEL_WIDTH) {
            MAX_PRIORITY_LABEL_WIDTH = label_width;
        }
    }

    // Log the calculation for debugging
    log_this("Configuration", "Priority label width calculated: %d", 
             LOG_LEVEL_DEBUG, MAX_PRIORITY_LABEL_WIDTH);
}