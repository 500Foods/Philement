/*
 * Priority level management for configuration system
 * 
 * This module handles priority level definitions and calculations used
 * throughout the configuration system. It provides:
 * 
 * 1. Priority Level Management
 *    - Standard priority level definitions
 *    - Label width calculations
 *    - Consistent formatting
 * 
 * 2. Label Formatting
 *    - Pre-calculated widths for alignment
 *    - Dynamic width adjustment
 *    - Consistent output formatting
 * 
 * Why This Design:
 * - Centralizes priority management
 * - Ensures consistent formatting
 * - Supports dynamic priority systems
 * - Maintains log readability
 */

#ifndef CONFIG_PRIORITY_H
#define CONFIG_PRIORITY_H

// Number of priority levels in the system
#define NUM_PRIORITY_LEVELS 7

// Priority level structure
typedef struct {
    int value;
    const char* label;
} PriorityLevel;

// Global label width for formatting
extern int MAX_PRIORITY_LABEL_WIDTH;
extern int MAX_SUBSYSTEM_LABEL_WIDTH;

// Default priority levels
extern const PriorityLevel DEFAULT_PRIORITY_LEVELS[NUM_PRIORITY_LEVELS];

/*
 * Calculate maximum width of priority labels
 * 
 * Pre-calculates label widths to ensure consistent log formatting.
 * This avoids repeated calculations and maintains consistent output.
 * 
 * Updates the global MAX_PRIORITY_LABEL_WIDTH variable based on
 * the longest label in DEFAULT_PRIORITY_LEVELS.
 * 
 * Thread Safety:
 * - Not thread safe, should only be called during initialization
 * - Modifies global state (MAX_PRIORITY_LABEL_WIDTH)
 */
void calculate_max_priority_label_width(void);

#endif /* CONFIG_PRIORITY_H */