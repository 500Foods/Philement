/*
 * Configuration file utilities
 * 
 * Provides standardized file operations for the configuration system:
 * - File existence and readability checks
 * - Common file operation patterns
 * - Error handling for file operations
 */

#ifndef CONFIG_FILE_UTILS_H
#define CONFIG_FILE_UTILS_H

#include <stdbool.h>

/*
 * Helper function to check if a file exists and is readable
 * 
 * @param path The path to check
 * @return true if the file exists and is readable, false otherwise
 */
bool is_file_readable(const char* path);

#endif /* CONFIG_FILE_UTILS_H */