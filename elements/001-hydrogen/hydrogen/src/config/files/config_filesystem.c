/*
 * Filesystem operations for configuration management
 * 
 * Why This Architecture:
 * 1. Safety-Critical Design
 *    - All operations handle system call failures
 *    - Memory allocation failures handled gracefully
 *    - Proper cleanup on error paths
 *    - No global state or side effects
 * 
 * 2. Performance Considerations
 *    - Uses stat() to avoid unnecessary file opens
 *    - Minimizes system calls
 *    - Efficient string handling
 *    - Fixed-size buffers where appropriate
 * 
 * 3. Maintainability
 *    - Clear error reporting
 *    - Consistent logging
 *    - Well-documented error paths
 *    - Separation of concerns
 * 
 * Implementation Notes:
 * - No private static functions needed as each public function
 *   is self-contained and has a single, clear responsibility
 * - Error handling is standardized across all functions
 * - Memory management follows consistent patterns
 */

// Feature test macros must come first
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <linux/limits.h>
#include <errno.h>

// Standard C headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Project headers
#include "config_filesystem.h"
#include "../../logging/logging.h"

// Public interface
char* get_executable_path(void);
long get_file_size(const char* filename);
char* get_file_modification_time(const char* filename);

// Private declarations
// No private functions needed - each public function is self-contained

/*
 * Get executable location with robust error handling
 * 
 * Why use /proc/self/exe?
 * 1. Reliability
 *    - Works with symlinks
 *    - Handles SUID/SGID binaries
 *    - Provides absolute path
 * 
 * 2. Security
 *    - Race-condition free
 *    - Permission-safe
 *    - No assumptions about CWD
 * 
 * 3. Performance
 *    - Single system call
 *    - No path resolution needed
 *    - Minimal memory allocation
 * 
 * Error Handling:
 * - EACCES: Permission denied
 * - ENOENT: /proc not mounted
 * - ENOMEM: Memory allocation failed
 */
char* get_executable_path(void) {
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    
    if (len == -1) {
        log_this("Configuration", "Error reading /proc/self/exe: %s", 
                 LOG_LEVEL_ERROR, strerror(errno));
        return NULL;
    }
    
    path[len] = '\0';
    char* result = strdup(path);
    
    if (!result) {
        log_this("Configuration", "Memory allocation failed for executable path", 
                 LOG_LEVEL_ERROR);
        return NULL;
    }
    
    return result;
}

/*
 * Get file size with proper error detection
 * 
 * Why use stat()?
 * 1. Efficiency
 *    - No file open required
 *    - Single system call
 *    - Works with special files
 * 
 * 2. Safety
 *    - Race-condition resistant
 *    - No file descriptor leaks
 *    - Handles all file types
 * 
 * 3. Reliability
 *    - Atomic size reading
 *    - Consistent behavior
 *    - Clear error reporting
 * 
 * Error Handling:
 * - ENOENT: File does not exist
 * - EACCES: Permission denied
 * - ELOOP: Too many symlinks
 */
long get_file_size(const char* filename) {
    struct stat st;
    
    if (!filename) {
        log_this("Configuration", "NULL filename passed to get_file_size", 
                 LOG_LEVEL_ERROR);
        return -1;
    }
    
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    
    log_this("Configuration", "Error getting size of %s: %s", 
             LOG_LEVEL_ERROR, filename, strerror(errno));
    return -1;
}

/*
 * Get file modification time in human-readable format
 * 
 * Why this format?
 * 1. Consistency
 *    - ISO 8601-like timestamps
 *    - Fixed width output
 *    - Standard separators
 * 
 * 2. Usability
 *    - Human readable
 *    - Local time for admins
 *    - Complete date context
 * 
 * 3. Reliability
 *    - Safe string handling
 *    - Clear error paths
 *    - Memory management
 * 
 * Error Handling:
 * - ENOMEM: Memory allocation failed
 * - ENOENT: File does not exist
 * - EACCES: Permission denied
 */
char* get_file_modification_time(const char* filename) {
    struct stat st;
    
    if (!filename) {
        log_this("Configuration", "NULL filename passed to get_file_modification_time", 
                 LOG_LEVEL_ERROR);
        return NULL;
    }
    
    if (stat(filename, &st) != 0) {
        log_this("Configuration", "Error getting stats for %s: %s", 
                 LOG_LEVEL_ERROR, filename, strerror(errno));
        return NULL;
    }

    struct tm* tm_info = localtime(&st.st_mtime);
    if (!tm_info) {
        log_this("Configuration", "Error converting time for %s", 
                 LOG_LEVEL_ERROR, filename);
        return NULL;
    }

    // YYYY-MM-DD HH:MM:SS\0
    char* time_str = malloc(20);
    if (!time_str) {
        log_this("Configuration", "Memory allocation failed for time string", 
                 LOG_LEVEL_ERROR);
        return NULL;
    }

    if (strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info) == 0) {
        log_this("Configuration", "Error formatting time for %s", 
                 LOG_LEVEL_ERROR, filename);
        free(time_str);
        return NULL;
    }

    return time_str;
}