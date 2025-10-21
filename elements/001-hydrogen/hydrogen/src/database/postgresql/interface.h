/*
 * PostgreSQL Database Engine - Interface Header
 *
 * Header file for PostgreSQL engine interface functions.
 */

#ifndef DATABASE_ENGINE_POSTGRESQL_INTERFACE_H
#define DATABASE_ENGINE_POSTGRESQL_INTERFACE_H

#include <src/database/database.h>

// Function prototype for the main interface function
DatabaseEngineInterface* postgresql_get_interface(void);

#endif // DATABASE_ENGINE_POSTGRESQL_INTERFACE_H