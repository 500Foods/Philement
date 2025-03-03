/*
 * Library dependency checking utilities
 *
 * Provides functionality for:
 * - Checking required library dependencies
 * - Comparing expected vs. runtime versions
 * - Reporting library status with appropriate severity
 */

#ifndef UTILS_DEPENDENCY_H
#define UTILS_DEPENDENCY_H

#include <stdbool.h>
#include "../config/configuration.h"

// Status values for library dependencies
typedef enum {
    LIB_STATUS_GOOD,         // Version matches or newer compatible version
    LIB_STATUS_WARNING,      // Different version but likely compatible
    LIB_STATUS_CRITICAL,     // Missing or incompatible version
    LIB_STATUS_UNKNOWN       // Unable to determine status
} LibraryStatus;

// Structure to hold library dependency information
typedef struct {
    const char *name;         // Library name
    const char *expected;     // Expected version
    const char *found;        // Found version at runtime (or "None")
    LibraryStatus status;     // Status enum
    bool is_required;         // Whether this library is required
} LibraryDependency;

// Check all library dependencies based on the current configuration
// Returns the number of critical dependencies that were missing
int check_library_dependencies(const AppConfig *config);

// Check an individual library dependency
// Determines if it's available and its version, then logs the result
void check_library_dependency(const char *name, const char *expected, bool is_required);

#endif // UTILS_DEPENDENCY_H