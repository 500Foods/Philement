/*
 * SQLite Database Engine - Interface Header
 *
 * Header file for SQLite engine interface functions.
 */

#ifndef SQLITE_INTERFACE_H
#define SQLITE_INTERFACE_H

#include <src/database/database.h>

// Function prototype for the main interface function
DatabaseEngineInterface* sqlite_get_interface(void);

#endif // SQLITE_INTERFACE_H