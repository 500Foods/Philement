/*
 * Firebird Database Engine - Query Execution Header
 *
 * Header file for Firebird query execution functions.
 * Phase 5 skeleton: execute_query and execute_prepared return false /
 * DB_QUERY_RESULT_UNSUPPORTED. Phase 6 implements real isc_dsql_* calls.
 */

#ifndef DATABASE_ENGINE_FIREBIRD_QUERY_H
#define DATABASE_ENGINE_FIREBIRD_QUERY_H

#include <src/database/database.h>
#include <src/database/database_params.h>

/*
 * Query execution.
 * Phase 5: these return false (skeleton). The QueryResult is allocated
 * and the caller owns its lifetime via database_engine_cleanup_result().
 */
bool firebird_execute_query(DatabaseHandle* connection,
                             QueryRequest* request,
                             QueryResult** result);

bool firebird_execute_prepared(DatabaseHandle* connection,
                               const PreparedStatement* stmt,
                               QueryRequest* request,
                               QueryResult** result);

#endif // DATABASE_ENGINE_FIREBIRD_QUERY_H
