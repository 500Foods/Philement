/*
 * SQLite Database Engine Header
 *
 * Header file for SQLite engine functions.
 */

#ifndef DATABASE_ENGINE_SQLITE_SQLITE_H
#define DATABASE_ENGINE_SQLITE_SQLITE_H

// Engine version and information functions for testing and coverage
const char* sqlite_engine_get_version(void);
bool sqlite_engine_is_available(void);
const char* sqlite_engine_get_description(void);
__attribute__((unused)) void sqlite_engine_test_functions(void);

#endif // DATABASE_ENGINE_SQLITE_SQLITE_H