/*
 * Library dependency checking and dynamic loading utilities
 *
 * Provides functionality for:
 * - Checking required library dependencies
 * - Comparing expected vs. runtime versions
 * - Reporting library status with appropriate severity
 * - Dynamically loading optional libraries only when needed
 * - Gracefully handling missing libraries with fallback mechanisms
 */

#ifndef UTILS_DEPENDENCY_H
#define UTILS_DEPENDENCY_H

#include <stdbool.h>
#include <dlfcn.h>
#include <src/config/config_forward.h>  // For AppConfig type
#include <src/config/config.h>          // For config functions

// Status values for library dependencies
typedef enum {
    LIB_STATUS_GOOD,         // Version matches or newer compatible version
    LIB_STATUS_WARNING,      // Different version but likely compatible
    LIB_STATUS_CRITICAL,     // Missing or incompatible version
    LIB_STATUS_UNKNOWN       // Unable to determine status
} LibraryStatus;

// Library handle for dynamically loaded libraries
typedef struct {
    void *handle;            // Handle returned by dlopen()
    bool is_loaded;          // Whether the library is currently loaded
    char *name;              // Library name
    char *version;           // Library version
    LibraryStatus status;    // Current status
} LibraryHandle;

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

// Dynamically load a library and return a handle to it
// If the library is not available, returns a handle with is_loaded=false
// The library is loaded with RTLD_LAZY and RTLD_LOCAL flags unless otherwise specified
// Returns: LibraryHandle with populated information
LibraryHandle *load_library(const char *lib_name, int dlopen_flags);

// Unload a previously loaded library
// Returns: true if successful, false if failed
bool unload_library(LibraryHandle *handle);

// Get a function pointer from a dynamically loaded library
// Returns: Function pointer if successful, NULL if failed
void *get_library_function(LibraryHandle *handle, const char *function_name);

// Check if a library is available without actually loading it
// Returns: true if library is available, false if not
bool is_library_available(const char *lib_name);

// Function type for generic callbacks to enable ISO C compliant callbacks
typedef void (*GenericCallback)(void);

// Helper functions that implement the dynamic loading functionality in ISO C compliant way
void *get_function_pointer(LibraryHandle *handle, const char *func_name);

// Helper macro to safely call a dynamically loaded function with fallback
// Uses functions instead of GNU extensions for ISO C compliance
// Args: handle (LibraryHandle*), func_name (string), fallback (code to run if function not available)
//       ... (arguments to pass to the function)
#define CALL_LIB_FUNCTION_WITH_FALLBACK(handle, func_name, fallback_code, ret_type, ...) \
    call_lib_function_helper((handle), (func_name), (ret_type)0, ##__VA_ARGS__)

// Helper function to make function calls in an ISO C compliant way
// This is used by the CALL_LIB_FUNCTION_WITH_FALLBACK macro
void *call_lib_function_helper(LibraryHandle *handle, const char *func_name, ...);

// Helper macro for void functions
// Uses a do-while loop which is ISO C compliant
// Args: handle (LibraryHandle*), func_name (string), fallback (code to run if function not available),
//       ... (arguments to pass to the function)
#define CALL_LIB_VOID_FUNCTION_WITH_FALLBACK(handle, func_name, fallback_code, ...) \
    do { \
        void* func_ptr = NULL; \
        if ((handle) && (handle)->is_loaded) { \
            func_ptr = get_library_function((handle), (func_name)); \
            if (func_ptr) { \
                /* Call function through valid opaque pointer */ \
                call_lib_void_function_helper(func_ptr, ##__VA_ARGS__); \
            } else { \
                fallback_code; \
            } \
        } else { \
            fallback_code; \
        } \
    } while(0)

// Helper function to make void function calls in an ISO C compliant way
void call_lib_void_function_helper(void *func_ptr, ...);

// Internal cache functions (made non-static for testing)
char* get_cache_file_path(const char *db_name);
bool ensure_cache_dir(void);
const char* load_cached_version(const char *db_name, char *buffer, size_t size);
void save_cache(const char *db_name, const char *version);

#endif // UTILS_DEPENDENCY_H
