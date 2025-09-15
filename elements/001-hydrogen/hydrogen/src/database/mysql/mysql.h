/*
 * MySQL Database Engine Header
 *
 * Header file for MySQL engine functions.
 */

#ifndef DATABASE_ENGINE_MYSQL_MYSQL_H
#define DATABASE_ENGINE_MYSQL_MYSQL_H

// Engine version and information functions for testing and coverage
const char* mysql_engine_get_version(void);
bool mysql_engine_is_available(void);
const char* mysql_engine_get_description(void);
__attribute__((unused)) void mysql_engine_test_functions(void);

#endif // DATABASE_ENGINE_MYSQL_MYSQL_H