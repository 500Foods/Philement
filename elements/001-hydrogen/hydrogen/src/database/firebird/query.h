/*
 * Firebird Database Engine - Query Execution Header
 */

#ifndef DATABASE_ENGINE_FIREBIRD_QUERY_H
#define DATABASE_ENGINE_FIREBIRD_QUERY_H

#include <src/database/database.h>
#include <src/database/database_params.h>

QueryResult* firebird_build_error_result(const char* error_msg, DatabaseErrorClass err_class);

bool firebird_execute_sql(DatabaseHandle* connection,
                          const char* sql,
                          const char* parameters_json,
                          QueryResult** result);

bool firebird_execute_query(DatabaseHandle* connection,
                             QueryRequest* request,
                             QueryResult** result);

bool firebird_execute_prepared(DatabaseHandle* connection,
                               const PreparedStatement* stmt,
                               QueryRequest* request,
                               QueryResult** result);

/* Temporal JSON + DATEADD(- ?) rewrite (Test 40 auth). */
bool firebird_append_temporal_json(char** buf, size_t* size, size_t* cap,
                                   short typ, const char* sqldata, short sqllen);
char* firebird_rewrite_dateadd_params(const char* sql, bool* oom);

#endif /* DATABASE_ENGINE_FIREBIRD_QUERY_H */
