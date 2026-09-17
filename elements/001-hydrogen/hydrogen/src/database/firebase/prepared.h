/*
 * Firebase engine - prepared statements (Phase 3: not implemented).
 */

#ifndef DATABASE_ENGINE_FIREBASE_PREPARED_H
#define DATABASE_ENGINE_FIREBASE_PREPARED_H

#include <src/database/database.h>

bool firebase_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql,
                                PreparedStatement** stmt, bool add_to_cache);
bool firebase_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);

#endif /* DATABASE_ENGINE_FIREBASE_PREPARED_H */
