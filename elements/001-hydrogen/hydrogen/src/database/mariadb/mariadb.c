/*
 * MariaDB Database Engine Implementation
 *
 * Implements the MariaDB database engine for the Hydrogen database subsystem.
 * Uses dynamic loading (dlopen/dlsym) for libmariadb to avoid static linking dependencies.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

// Local includes
#include "types.h"
#include "interface.h"

// Function prototypes
const char* mariadb_engine_get_version(void);
bool mariadb_engine_is_available(void);
const char* mariadb_engine_get_description(void);
__attribute__((unused)) void mariadb_engine_test_functions(void);

// Engine version and information functions for testing and coverage
const char* mariadb_engine_get_version(void) {
    return "MariaDB Engine v1.0.0";
}

bool mariadb_engine_is_available(void) {
    // Try to load the MariaDB library
    void* test_handle = dlopen("libmariadb.so.3", RTLD_LAZY);
    if (!test_handle) {
        test_handle = dlopen("libmariadb.so", RTLD_LAZY);
    }

    if (test_handle) {
        dlclose(test_handle);
        return true;
    }

    return false;
}

const char* mariadb_engine_get_description(void) {
    return "MariaDB / MySQL Compatible Supported";
}

// Use the functions to avoid unused warnings (for testing/coverage purposes)
__attribute__((unused)) void mariadb_engine_test_functions(void) {
    const char* version = mariadb_engine_get_version();
    bool available = mariadb_engine_is_available();
    const char* description = mariadb_engine_get_description();

    // Prevent optimization from removing the calls
    (void)version;
    (void)available;
    (void)description;
}
