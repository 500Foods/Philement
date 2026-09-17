/*
 * Firebase engine - query execution (Phase 3: not implemented).
 */

#ifndef DATABASE_ENGINE_FIREBASE_QUERY_H
#define DATABASE_ENGINE_FIREBASE_QUERY_H

#include <src/database/database.h>

bool firebase_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool firebase_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt,
                               QueryRequest* request, QueryResult** result);

QueryResult* firebase_build_not_implemented_result(const char* message);

#endif /* DATABASE_ENGINE_FIREBASE_QUERY_H */
