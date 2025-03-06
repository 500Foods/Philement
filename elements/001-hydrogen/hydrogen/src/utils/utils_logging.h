/*
 * ID generation and logging utilities.
 * 
 * Provides functionality for:
 * - ID generation for unique identifiers
 * - Log message formatting
 * - Priority label handling
 * 
 * Note: Console logging functionality has been moved to logging.h
 * to maintain proper separation of concerns and avoid circular dependencies.
 */

#ifndef UTILS_LOGGING_H
#define UTILS_LOGGING_H

#include <stddef.h>

// Constants for ID generation
#define ID_CHARS "BCDFGHKPRST"  // Consonants for readable IDs
#define ID_LEN 5                // Balance of uniqueness and readability

// Include configuration for label widths
#include "../config/config.h"

// ID generation function
void generate_id(char *buf, size_t len);

// Priority label handling
const char* get_priority_label(int priority);

#endif // UTILS_LOGGING_H