/*
 * Configuration file handling utilities
 * 
 * Provides filesystem operations for configuration processing:
 * - File readability checking
 * - Executable path discovery
 * - File size and modification time retrieval
 */

#ifndef CONFIG_FILES_H
#define CONFIG_FILES_H

#include <stdbool.h>

/*
 * Check if file is readable with proper error detection
 * 
 * Uses access() to check file readability:
 * - Efficient permission checking
 * - Works with different user contexts
 * - Follows symbolic links
 * - Atomic operation
 * 
 * @param path Path to the file to check
 * @return true if file exists and is readable
 * @return false if file doesn't exist, isn't readable, or isn't a regular file
 */
bool is_file_readable(const char* path);

/*
 * Get executable path with robust error handling
 * 
 * Uses /proc/self/exe to find the true binary path, which:
 * - Works with symlinks
 * - Handles SUID/SGID binaries
 * - Provides absolute path
 * - Works regardless of current directory
 * 
 * @return Newly allocated string with path
 * @return NULL on error, with specific error logged
 * The caller must free the returned string when no longer needed.
 */
char* get_executable_path(void);

/*
 * Get file size with proper error detection
 * 
 * Uses stat() to efficiently get file size:
 * - Avoids opening the file
 * - Works for special files
 * - More efficient than seeking
 * - Atomic size reading
 * 
 * @param filename Path to the file to check
 * @return File size in bytes on success
 * @return -1 on error, with specific error logged
 */
long get_file_size(const char* filename);

/*
 * Get file modification time in human-readable format
 * 
 * Formats the time as: YYYY-MM-DD HH:MM:SS
 * - ISO 8601-like timestamp for consistency
 * - Local time for admin readability
 * - Fixed width for log formatting
 * 
 * @param filename Path to the file to check
 * @return Newly allocated string with timestamp
 * @return NULL on error, with specific error logged
 * The caller must free the returned string when no longer needed.
 */
char* get_file_modification_time(const char* filename);

