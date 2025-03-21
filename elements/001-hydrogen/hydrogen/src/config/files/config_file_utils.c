/*
 * Configuration file utilities implementation
 */

// Feature test macros
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

// Core system headers
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// Project headers
#include "config_file_utils.h"

/*
 * Helper function to check if a file exists and is readable
 * 
 * @param path The path to check
 * @return true if the file exists and is readable, false otherwise
 */
bool is_file_readable(const char* path) {
    if (!path) return false;
    struct stat st;
    return (stat(path, &st) == 0 && S_ISREG(st.st_mode) && access(path, R_OK) == 0);
}