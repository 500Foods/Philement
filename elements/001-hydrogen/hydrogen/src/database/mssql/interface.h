/*
 * MSSQL Database Engine - Interface Header
 *
 * Header file for MSSQL engine interface functions.
 */

#ifndef DATABASE_ENGINE_MSSQL_INTERFACE_H
#define DATABASE_ENGINE_MSSQL_INTERFACE_H

#include <src/database/database.h>

// Function prototype for the main interface function
DatabaseEngineInterface* mssql_get_interface(void);

// Engine information functions (for testing)
const char* mssql_engine_get_version(void);
bool mssql_engine_is_available(void);
const char* mssql_engine_get_description(void);
void mssql_engine_test_functions(void);

#endif // DATABASE_ENGINE_MSSQL_INTERFACE_H
