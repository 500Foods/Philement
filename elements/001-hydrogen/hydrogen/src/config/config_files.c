/*
 * Configuration file handling utilities implementation
 * 
 * Provides filesystem operations for configuration processing:
 * - File readability checking
 * - Executable path discovery
 * - File size and modification time retrieval
 */

// Global includes 
#include <src/hydrogen.h>

// Local includes
#include "config_files.h"

/*
 * Filesystem Operations Implementation
 * These functions provide safe, consistent filesystem access
 * with proper error handling and memory management.
 */

bool is_file_readable(const char* path) {
    if (!path) {
        log_this(SR_CONFIG, "NULL path passed to is_file_readable", LOG_LEVEL_ERROR, 0);
        return false;
    }
    
    // Check if file exists and is readable
    if (access(path, R_OK) == 0) {
        struct stat st;
        if (stat(path, &st) == 0) {
            // Verify it's a regular file
            if (S_ISREG(st.st_mode)) {
                return true;
            }
            log_this(SR_CONFIG, "Path exists but is not a regular file: %s", LOG_LEVEL_ERROR, 1, path);
            return false;
        }
        log_this(SR_CONFIG, "Failed to stat file %s: %s", LOG_LEVEL_ERROR, 2, path, strerror(errno));
        return false;
    }
    
    // Only log as error if file exists but isn't readable
    if (access(path, F_OK) == 0) {
        log_this(SR_CONFIG, "File exists but is not readable: %s", LOG_LEVEL_ERROR, 1, path);
    }
    return false;
}

char* get_executable_path(void) {
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    
    if (len == -1) {
        log_this(SR_CONFIG, "Error reading /proc/self/exe: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
        return NULL;
    }
    
    if (len < 0) {
        log_this(SR_CONFIG, "Invalid length from readlink", LOG_LEVEL_ERROR, 0);
        return NULL;
    }
    
    path[(size_t)len] = '\0';
    char* result = strdup(path);
    
    if (!result) {
        log_this(SR_CONFIG, "Memory allocation failed for executable path", LOG_LEVEL_ERROR, 0);
        return NULL;
    }
    
    return result;
}

long get_file_size(const char* filename) {
    struct stat st;
    
    if (!filename) {
        log_this(SR_CONFIG, "NULL filename passed to get_file_size", LOG_LEVEL_ERROR, 0);
        return -1;
    }
    
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    
    log_this(SR_CONFIG, "Error getting size of %s: %s", LOG_LEVEL_ERROR, 2, filename, strerror(errno));
    return -1;
}

char* get_file_modification_time(const char* filename) {
    struct stat st;
    
    if (!filename) {
        log_this(SR_CONFIG, "NULL filename passed to get_file_modification_time", LOG_LEVEL_ERROR, 0);
        return NULL;
    }
    
    if (stat(filename, &st) != 0) {
        log_this(SR_CONFIG, "Error getting stats for %s: %s", LOG_LEVEL_ERROR, 2, filename, strerror(errno));
        return NULL;
    }

    const struct tm* tm_info = localtime(&st.st_mtime);
    if (!tm_info) {
        log_this(SR_CONFIG, "Error converting time for %s", LOG_LEVEL_ERROR, 1, filename);
        return NULL;
    }

    // YYYY-MM-DD HH:MM:SS\0
    char* time_str = malloc(20);
    if (!time_str) {
        log_this(SR_CONFIG, "Memory allocation failed for time string", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    if (strftime(time_str, 20, "%Y-%m-%d %H:%M:%S", tm_info) == 0) {
        log_this(SR_CONFIG, "Error formatting time for %s", LOG_LEVEL_ERROR, 1, filename);
        free(time_str);
        return NULL;
    }

    return time_str;
}
