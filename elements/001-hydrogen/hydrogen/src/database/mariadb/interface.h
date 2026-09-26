/*
 * MariaDB Database Engine - Interface Header
 *
 * Header file for MariaDB engine interface functions.
 */

#ifndef DATABASE_ENGINE_MARIADB_INTERFACE_H
#define DATABASE_ENGINE_MARIADB_INTERFACE_H

#include <src/database/database.h>

// Function prototype for the main interface function
DatabaseEngineInterface* mariadb_get_interface(void);

#endif // DATABASE_ENGINE_MARIADB_INTERFACE_H
