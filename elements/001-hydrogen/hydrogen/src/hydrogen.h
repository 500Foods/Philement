/* hydrogen.h
 *
 * This is intended to be a global header file used throughout the project
 * to provide common definitions, includes, and utility functions, particularly
 * for supporting multiple platforms - primarily Linux, macOS, and Windows.
 *
 */

#ifndef HYDROGEN_H
#define HYDROGEN_H

// Feature test macros (must be first, before any includes)
#ifndef _POSIX_C_SOURCE
    #define _POSIX_C_SOURCE 200809L
#endif

// Platform detection and specific defines
#if defined(__linux__)
    #define PLATFORM_LINUX

    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE
    #endif

    #include <linux/limits.h>

#elif defined(__APPLE__)
    #define PLATFORM_MACOS

    // Avoid deprecated warnings for certain functions if using stricter flags
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"

    #include <sys/syslimits.h>

    // reallocarray is missing on macOS; provide a fallback
    #ifndef reallocarray
        #include <errno.h>   // For ENOMEM
        #include <stddef.h>  // For SIZE_MAX
        #include <stdlib.h>  // For realloc (include early here)

        static inline void* reallocarray(void* ptr, size_t nmemb, size_t size) {
            // Check for overflow
            if (size && nmemb > SIZE_MAX / size) {
                errno = ENOMEM;
                return NULL;
            }
            return realloc(ptr, nmemb * size);
        }
    #endif

#elif defined(_WIN32)
    #define PLATFORM_WINDOWS

    // Windows-specific includes and defines (e.g., for limits)
    #include <limits.h>
    #include <windows.h>  // If needed for WinAPI
    
#else
    #error "Unsupported platform"
#endif

// Standard C library includes
#include <ctype.h>          // Character classification/conversion (e.g., isalpha, tolower)
#include <errno.h>          // Error codes and errno variable (e.g., perror, strerror)
#include <libgen.h>         // Pathname utilities (e.g., basename, dirname; POSIX)
#include <signal.h>         // Signal handling (e.g., signal, sigaction)
#include <stdbool.h>        // Boolean type and constants (bool, true, false)
#include <stddef.h>         // Standard types and macros (e.g., size_t, NULL, offsetof)
#include <stdint.h>         // Fixed-width integer types (e.g., uint8_t, int64_t)
#include <stdio.h>          // Formatted I/O functions (e.g., printf, scanf, fopen)
#include <stdlib.h>         // General utilities (e.g., malloc, free, atoi, exit, rand)
#include <stdarg.h>         // Variable argument lists (e.g., va_list, va_start for variadic functions)
#include <string.h>         // String manipulation (e.g., strcpy, strlen, memcmp)
#include <strings.h>        // Case-insensitive/BSD string functions (e.g., strcasecmp, bcopy; POSIX extension)
#include <time.h>           // Time/date manipulation (e.g., time, localtime, strftime)

// POSIX-specific includes
#include <sys/types.h>      // POSIX types (e.g., pid_t, ssize_t, off_t; often required before other sys/ headers)
#include <sys/stat.h>       // File status and modes (e.g., stat, fstat, mkdir, chmod; for file metadata)
#include <unistd.h>         // POSIX functions (e.g., fork, exec, getpid, sleep, access; for system calls and utilities)
#include <sys/utsname.h>    // System information (e.g., uname for OS name, version, architecture; POSIX)

// Thid-party libraries
#include <jansson.h>        // For jansson (JSON)
#include <libwebsockets.h>  // For websockets
#include <microhttpd.h>     // For microhttpd
#include <pthread.h>        // For pthreads

// Project includes
#include "config/config.h"
#include "logging/logging.h"
#include "network/network.h"
#include "payload/payload.h"
#include "registry/registry.h"
#include "state/state.h"
#include "status/status.h"
#include "threads/threads.h"
#include "utils/utils.h"

#endif /* HYDROGEN_H */
