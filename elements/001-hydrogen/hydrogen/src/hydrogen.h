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
#include <dirent.h>         // Directory operations (e.g., opendir, readdir; POSIX)
#include <dlfcn.h>          // Dynamic linking (e.g., dlopen, dlsym; POSIX)
#include <errno.h>          // Error codes and errno variable (e.g., perror, strerror)
#include <fcntl.h>          // File control options (e.g., open, O_RDONLY; POSIX)
#include <features.h>       // GNU/glibc features 
#include <libgen.h>         // Pathname utilities (e.g., basename, dirname; POSIX)
#include <locale.h>         // Locale settings 
#include <math.h>           // Mathematical functions (e.g., sin, cos, sqrt)
#include <mntent.h>         // Filesystem mount points (e.g., getmntent; POSIX)
#include <regex.h>          // Regular expression handling
#include <semaphore.h>      // Mutex lock handling
#include <signal.h>         // Signal handling (e.g., signal, sigaction)
#include <stdarg.h>         // Variable argument lists (e.g., va_list, va_start for variadic functions)
#include <stdbool.h>        // Boolean type and constants (bool, true, false)
#include <stddef.h>         // Standard types and macros (e.g., size_t, NULL, offsetof)
#include <stdint.h>         // Fixed-width integer types (e.g., uint8_t, int64_t)
#include <stdio.h>          // Formatted I/O functions (e.g., printf, scanf, fopen)
#include <stdlib.h>         // General utilities (e.g., malloc, free, atoi, exit, rand)
#include <string.h>         // String manipulation (e.g., strcpy, strlen, memcmp)
#include <strings.h>        // Case-insensitive/BSD string functions (e.g., strcasecmp, bcopy; POSIX extension)
#include <time.h>           // Time/date manipulation (e.g., time, localtime, strftime)

// POSIX-specific includes
#include <sys/ioctl.h>      // I/O control operations (e.g., ioctl; POSIX)
#include <sys/mman.h>       // Memory management declarations (e.g., mmap, munmap; POSIX)
#include <sys/prctl.h>      // Process control 
#include <sys/resource.h>   // Resource limits (e.g., getrlimit, setrlimit; POSIX)
#include <sys/stat.h>       // File status and modes (e.g., stat, fstat, mkdir, chmod; for file metadata)
#include <sys/time.h>       // Time-related functions (e.g., gettimeofday, clock_gettime; POSIX)
#include <sys/types.h>      // POSIX types (e.g., pid_t, ssize_t, off_t; often required before other sys/ headers)
#include <sys/utsname.h>    // System information (e.g., uname for OS name, version, architecture; POSIX)
#include <unistd.h>         // POSIX functions (e.g., fork, exec, getpid, sleep, access; for system calls and utilities)

// Thid-party libraries
#include <jansson.h>        // For jansson - JSON handling
#include <libwebsockets.h>  // For websockets - WebSocket server/client
#include <microhttpd.h>     // For microhttpd - Web server
#ifndef USE_MOCK_PTHREAD
#include <pthread.h>        // For pthreads - Threading and synchronization
#endif
#ifdef USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>
#endif
#include <brotli/decode.h>  // For Brotli - Decompressing payloads
#include <brotli/encode.h>  // For Brotli - Compressing content

// Project constants
#include <src/globals.h>

// Project includes
#include <src/config/config.h>
#include <src/logging/logging.h>
#include <src/mutex/mutex.h>
#include <src/network/network.h>
#include <src/payload/payload.h>
#include <src/queue/queue.h>
#include <src/registry/registry.h>
#include <src/state/state.h>
#include <src/status/status.h>
#include <src/threads/threads.h>
#include <src/utils/utils.h>

// Main application configuration structure
// NOTE: Network-Notify are all primary subsystems that are also
// represented directly in config. Registry, Payload, and Threads 
// are the remaining subsystems with no direct config analog
struct AppConfig {
    ServerConfig server;           // A. Server configuration
    NetworkConfig network;         // B. Network configuration
    DatabaseConfig databases;      // C. Database configuration
    LoggingConfig logging;         // D. Logging configuration
    WebServerConfig webserver;     // E. WebServer configuration
    APIConfig api;                 // F. API configuration
    SwaggerConfig swagger;         // G. Swagger configuration
    WebSocketConfig websocket;     // H. WebSocket configuration
    TerminalConfig terminal;       // I. Terminal configuration
    MDNSServerConfig mdns_server;  // J. mDNS Server configuration
    MDNSClientConfig mdns_client;  // K. mDNS Client configuration
    MailRelayConfig mail_relay;    // L. Mail Relay configuration
    PrintConfig print;             // M. Print configuration
    ResourceConfig resources;      // N. Resources configuration
    OIDCConfig oidc;               // O. OIDC configuration
    NotifyConfig notify;           // P. Notify configuration
};

// Defined in global.c
extern AppConfig *app_config;

// Defined in global.c
extern long long server_executable_size;
void get_executable_size(char *argv[]);

// Defined in global.c
extern size_t registry_registered;
extern size_t registry_running;
extern size_t registry_attempted;
extern size_t registry_failed;

// Defined in state/state.c
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t server_running;

#endif /* HYDROGEN_H */
