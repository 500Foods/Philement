/*
 * Filesystem operations for configuration management
 * 
 * This module provides filesystem utility functions used by the configuration system.
 * 
 * Why This Design:
 * 1. Separation of Concerns
 *    - Isolates filesystem operations from configuration logic
 *    - Makes testing and mocking easier
 *    - Simplifies error handling for file operations
 * 
 * 2. Memory Safety
 *    - All functions handle allocation failures
 *    - Clear ownership rules for returned memory
 *    - Consistent error reporting
 * 
 * 3. Thread Safety
 *    - All functions are thread-safe
 *    - Uses thread-safe system calls
 *    - No global state
 */

#ifndef CONFIG_FILESYSTEM_H
#define CONFIG_FILESYSTEM_H

// Core system headers
#include <sys/types.h>

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
 * @return NULL on error, with specific error logged:
 *         - ENOMEM: Memory allocation failed
 *         - EACCES: Permission denied
 *         - ENOENT: /proc not mounted
 * 
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
 * @return -1 on error, with specific error logged:
 *         - ENOENT: File does not exist
 *         - EACCES: Permission denied
 *         - ELOOP: Too many symlinks
 */
long get_file_size(const char* filename);

/*
 * Get file modification time in human-readable format
 * 
 * Formats the time as: YYYY-MM-DD HH:MM:SS
 * - ISO 8601-like timestamp for consistency
 * - Local time for admin readability
 * - Fixed width for log formatting
 * - Complete date and time context
 * 
 * @param filename Path to the file to check
 * @return Newly allocated string with timestamp (YYYY-MM-DD HH:MM:SS format)
 * @return NULL on error, with specific error logged:
 *         - ENOMEM: Memory allocation failed
 *         - ENOENT: File does not exist
 *         - EACCES: Permission denied
 * 
 * The caller must free the returned string when no longer needed.
 */
char* get_file_modification_time(const char* filename);

#endif /* CONFIG_FILESYSTEM_H */