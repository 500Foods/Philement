/*
 * SQLite Database Engine - Connection Management Header
 *
 * Header file for SQLite connection management functions.
 */

#ifndef SQLITE_CONNECTION_H
#define SQLITE_CONNECTION_H

#include "../database.h"

// Connection management
bool sqlite_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
bool sqlite_disconnect(DatabaseHandle* connection);
bool sqlite_health_check(DatabaseHandle* connection);
bool sqlite_reset_connection(DatabaseHandle* connection);

#endif // SQLITE_CONNECTION_H