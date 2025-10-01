/*
 * Configuration file handling utilities
 */

#ifndef CONFIG_FILES_H
#define CONFIG_FILES_H

#include <stdbool.h>

/*
 * Check if file is readable with proper error detection
 */
bool is_file_readable(const char* path);

/*
 * Get executable path with robust error handling
 */
char* get_executable_path(void);

/*
 * Get file size with proper error detection
 */
long get_file_size(const char* filename);

/*
 * Get file modification time in human-readable format
 */
char* get_file_modification_time(const char* filename);

#endif /* CONFIG_FILES_H */
