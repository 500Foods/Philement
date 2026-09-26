/*
 * MariaDB Database Engine Header
 *
 * Header file for MariaDB engine functions.
 */

#ifndef DATABASE_ENGINE_MARIADB_MARIADB_H
#define DATABASE_ENGINE_MARIADB_MARIADB_H

// Engine version and information functions for testing and coverage
const char* mariadb_engine_get_version(void);
bool mariadb_engine_is_available(void);
const char* mariadb_engine_get_description(void);
__attribute__((unused)) void mariadb_engine_test_functions(void);

#endif // DATABASE_ENGINE_MARIADB_MARIADB_H
