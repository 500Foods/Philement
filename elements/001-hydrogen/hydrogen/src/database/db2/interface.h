/*
 * DB2 Database Engine - Interface Header
 *
 * Header file for DB2 engine interface functions.
 */

#ifndef DATABASE_ENGINE_DB2_INTERFACE_H
#define DATABASE_ENGINE_DB2_INTERFACE_H

#include <src/database/database.h>

// Function prototype for the main interface function
DatabaseEngineInterface* db2_get_interface(void);

// Engine information functions (for testing)
const char* db2_engine_get_version(void);
bool db2_engine_is_available(void);
const char* db2_engine_get_description(void);
void db2_engine_test_functions(void);

#endif // DATABASE_ENGINE_DB2_INTERFACE_H