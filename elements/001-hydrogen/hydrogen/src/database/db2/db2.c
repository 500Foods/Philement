/*
 * DB2 Database Engine Implementation
 *
 * Main entry point for the DB2 database engine.
 * This file serves as the main entry point for the DB2 database engine.
 * All implementation has been split into separate files for better organization.
 */

#include "../../hydrogen.h"
#include "../database.h"
#include "../database_queue.h"

// Include the interface header
#include "interface.h"

// Engine version and information functions for testing and coverage
static const char* db2_engine_get_version(void) {
    return "DB2 Engine v1.0.0";
}

static bool db2_engine_is_available(void) {
    // Try to load the DB2 library
    void* test_handle = dlopen("libdb2.so", RTLD_LAZY);
    if (!test_handle) {
        test_handle = dlopen("libdb2.so.1", RTLD_LAZY);
    }

    if (test_handle) {
        dlclose(test_handle);
        return true;
    }

    return false;
}

static const char* db2_engine_get_description(void) {
    return "DB2 database engine with dynamic loading support";
}

// Use the functions to avoid unused warnings (for testing/coverage purposes)
__attribute__((unused)) static void db2_engine_test_functions(void) {
    const char* version = db2_engine_get_version();
    bool available = db2_engine_is_available();
    const char* description = db2_engine_get_description();

    // Prevent optimization from removing the calls
    (void)version;
    (void)available;
    (void)description;
}