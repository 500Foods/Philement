/*
 * MSSQL Database Engine Implementation
 *
 * Main entry point for the MSSQL database engine.
 * This file serves as the main entry point for the MSSQL database engine.
 * All implementation has been split into separate files for better organization.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

// Local includes
#include "interface.h"

// Engine version and information functions for testing and coverage
const char* mssql_engine_get_version(void) {
    return "MSSQL Engine v1.0.0";
}

bool mssql_engine_is_available(void) {
    // Try to load the unixODBC library
    void* test_handle = dlopen("libodbc.so", RTLD_LAZY);
    if (!test_handle) {
        test_handle = dlopen("libodbc.so.2", RTLD_LAZY);
    }

    if (test_handle) {
        dlclose(test_handle);
        return true;
    }

    return false;
}

const char* mssql_engine_get_description(void) {
    return "Microsoft SQL Server 2022 Linux (unixODBC + ODBC Driver 18)";
}

// Use the functions to avoid unused warnings (for testing/coverage purposes)
__attribute__((unused)) void mssql_engine_test_functions(void) {
    const char* version = mssql_engine_get_version();
    bool available = mssql_engine_is_available();
    const char* description = mssql_engine_get_description();

    // Prevent optimization from removing the calls
    (void)version;
    (void)available;
    (void)description;
}
