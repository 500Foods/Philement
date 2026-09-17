/*
 * Firebase engine - connection, health, cancel.
 */

#ifndef DATABASE_ENGINE_FIREBASE_CONNECTION_H
#define DATABASE_ENGINE_FIREBASE_CONNECTION_H

#include <src/database/database.h>
#include "types.h"

void firebase_free_engine_connection(FirebaseConnection* fb);

bool firebase_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
bool firebase_disconnect(DatabaseHandle* connection);
bool firebase_health_check(DatabaseHandle* connection);
bool firebase_reset_connection(DatabaseHandle* connection);
void firebase_cancel_inflight(DatabaseHandle* connection);

#endif /* DATABASE_ENGINE_FIREBASE_CONNECTION_H */
