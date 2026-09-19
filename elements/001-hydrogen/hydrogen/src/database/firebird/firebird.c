/*
 * Firebird Database Engine Implementation
 *
 * Main entry point for the Firebird database engine.
 * This file serves as the main entry point for the Firebird database engine.
 * All implementation has been split into separate files for better organization.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

// Local includes
#include "interface.h"

/*
 * Engine metadata functions (for testing/coverage).
 * These are called by test suites and metrics collection.
 */
const char* firebird_engine_get_version(void) {
    return "Firebird Engine v0.1.0-skeleton (Phase 5)";
}

bool firebird_engine_is_available(void) {
#ifdef USE_MOCK_LIBFBC
    // In mock/test mode, always report available
    return true;
#else
    // Try to load the Firebird client library
    void* test_handle = dlopen("libfbclient.so.2", RTLD_LAZY);
    if (!test_handle) {
        test_handle = dlopen("libfbclient.so", RTLD_LAZY);
    }

    if (test_handle) {
        dlclose(test_handle);
        return true;
    }

    return false;
#endif
}

const char* firebird_engine_get_description(void) {
    return "Firebird SQL engine (skeleton Phase 5 — connect/disconnect/transaction only, no query execution)";
}

/*
 * Use the functions to avoid unused warnings (for testing/coverage purposes)
 */
__attribute__((unused)) void firebird_engine_test_functions(void) {
    const char* version = firebird_engine_get_version();
    bool available = firebird_engine_is_available();
    const char* description = firebird_engine_get_description();

    // Prevent optimization from removing the calls
    (void)version;
    (void)available;
    (void)description;
}
