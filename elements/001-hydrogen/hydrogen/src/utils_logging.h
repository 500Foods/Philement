/*
 * Logging and ID generation utilities.
 * 
 * Provides functionality for:
 * - Console logging
 * - ID generation
 * - Log message formatting
 * - Priority label handling
 */

#ifndef UTILS_LOGGING_H
#define UTILS_LOGGING_H

#include <stddef.h>

// Constants for ID generation
#define ID_CHARS "BCDFGHKPRST"  // Consonants for readable IDs
#define ID_LEN 5                // Balance of uniqueness and readability

// Include configuration for label widths
#include "configuration.h"

// Console logging function
void console_log(const char* subsystem, int priority, const char* message);

// ID generation function
void generate_id(char *buf, size_t len);

#endif // UTILS_LOGGING_H