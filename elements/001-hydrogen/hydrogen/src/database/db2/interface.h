/*
 * DB2 Database Engine - Interface Header
 *
 * Header file for DB2 engine interface functions.
 */

#ifndef DATABASE_ENGINE_DB2_INTERFACE_H
#define DATABASE_ENGINE_DB2_INTERFACE_H

#include "../database.h"

// Function prototype for the main interface function
DatabaseEngineInterface* db2_get_interface(void);

#endif // DATABASE_ENGINE_DB2_INTERFACE_H