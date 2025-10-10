/*
 * SQLite Database Engine Implementation
 *
 * Main entry point for the SQLite database engine.
 * This file serves as the main entry point for the SQLite database engine.
 * All implementation has been split into separate files for better organization.
 */

#include "../../hydrogen.h"
#include "../database.h"
#include "../queue/database_queue.h"

// Include the interface header
#include "interface.h"

// Function prototypes
const char* sqlite_engine_get_version(void);
bool sqlite_engine_is_available(void);
const char* sqlite_engine_get_description(void);
__attribute__((unused)) void sqlite_engine_test_functions(void);

// Engine version and information functions for testing and coverage
const char* sqlite_engine_get_version(void) {
    return "SQLite Engine v1.0.0";
}

bool sqlite_engine_is_available(void) {
    // Try to load the SQLite library
    void* test_handle = dlopen("libsqlite3.so", RTLD_LAZY);
    if (!test_handle) {
        test_handle = dlopen("libsqlite3.so.0", RTLD_LAZY);
    }

    if (test_handle) {
        dlclose(test_handle);
        return true;
    }

    return false;
}

const char* sqlite_engine_get_description(void) {
    return "SQLite Supported";
}

// Use the functions to avoid unused warnings (for testing/coverage purposes)
__attribute__((unused)) void sqlite_engine_test_functions(void) {
    const char* version = sqlite_engine_get_version();
    bool available = sqlite_engine_is_available();
    const char* description = sqlite_engine_get_description();

    // Prevent optimization from removing the calls
    (void)version;
    (void)available;
    (void)description;
}