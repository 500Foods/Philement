/*
 * MySQL Database Engine - Interface Header
 *
 * Header file for MySQL engine interface functions.
 */

#ifndef DATABASE_ENGINE_MYSQL_INTERFACE_H
#define DATABASE_ENGINE_MYSQL_INTERFACE_H

#include <src/database/database.h>

// Function prototype for the main interface function
DatabaseEngineInterface* mysql_get_interface(void);

#endif // DATABASE_ENGINE_MYSQL_INTERFACE_H