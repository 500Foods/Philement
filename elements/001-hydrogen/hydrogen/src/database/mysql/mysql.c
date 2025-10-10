/*
 * MySQL Database Engine Implementation
 *
 * Implements the MySQL database engine for the Hydrogen database subsystem.
 * Uses dynamic loading (dlopen/dlsym) for libmysqlclient to avoid static linking dependencies.
 */

#include "../../hydrogen.h"
#include "../database.h"
#include "../database_queue.h"
#include "types.h"

// Include the interface header
#include "interface.h"

// Function prototypes
const char* mysql_engine_get_version(void);
bool mysql_engine_is_available(void);
const char* mysql_engine_get_description(void);
__attribute__((unused)) void mysql_engine_test_functions(void);

// Engine version and information functions for testing and coverage
const char* mysql_engine_get_version(void) {
    return "MySQL Engine v1.0.0";
}

bool mysql_engine_is_available(void) {
    // Try to load the MySQL library
    void* test_handle = dlopen("libmysqlclient.so.21", RTLD_LAZY);
    if (!test_handle) {
        test_handle = dlopen("libmysqlclient.so.18", RTLD_LAZY);
    }
    if (!test_handle) {
        test_handle = dlopen("libmysqlclient.so.20", RTLD_LAZY);
    }
    if (!test_handle) {
        test_handle = dlopen("libmysqlclient.so", RTLD_LAZY);
    }

    if (test_handle) {
        dlclose(test_handle);
        return true;
    }

    return false;
}

const char* mysql_engine_get_description(void) {
    return "MySQL / MariaDB Supported";
}

// Use the functions to avoid unused warnings (for testing/coverage purposes)
__attribute__((unused)) void mysql_engine_test_functions(void) {
    const char* version = mysql_engine_get_version();
    bool available = mysql_engine_is_available();
    const char* description = mysql_engine_get_description();

    // Prevent optimization from removing the calls
    (void)version;
    (void)available;
    (void)description;
}