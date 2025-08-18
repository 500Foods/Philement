/*
 * Priority level management for configuration system
 * 
 */

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
 */
void calculate_max_priority_label_width(void) {
    MAX_PRIORITY_LABEL_WIDTH = 0;
    
    for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        int label_width = (int)strlen(DEFAULT_PRIORITY_LEVELS[i].label);
        if (label_width > MAX_PRIORITY_LABEL_WIDTH) {
            MAX_PRIORITY_LABEL_WIDTH = label_width;
        }
    }

    // Log the calculation for debugging
    log_this("Configuration", "Priority label width calculated: %d", 
             LOG_LEVEL_DEBUG, MAX_PRIORITY_LABEL_WIDTH);
}
