/*
 * Priority level management for configuration system
 */

#ifndef CONFIG_PRIORITY_H
#define CONFIG_PRIORITY_H

#include "../constants.h"

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

// Calculate maximum width of priority labels 
 void calculate_max_priority_label_width(void);

#endif /* CONFIG_PRIORITY_H */
