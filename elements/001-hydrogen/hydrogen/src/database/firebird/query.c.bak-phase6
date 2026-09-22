/*
 * Firebird Database Engine - Query Execution Implementation
 *
 * Implements the query execution hooks for the Firebird engine.
 *
 * Phase 5 skeleton: query execution is NOT implemented. These stubs
 * return false and allocate a QueryResult marked as unsupported
 * (DB_QUERY_RESULT_UNSUPPORTED is not a standard enum — we use
 * success=false with an error message). Phase 6 will implement
 * isc_dsql_allocate / prepare / execute / fetch and return real
 * QueryResult objects.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>

#include "types.h"
#include "connection.h"
#include "query.h"

/*
 * Execute a raw SQL query string against a Firebird connection.
 * Phase 5: returns false, result is NULL. Phase 6 will implement.
 */
bool firebird_execute_query(DatabaseHandle* connection,
                             QueryRequest* request,
                             QueryResult** result) {
    if (!connection || connection->engine_type != DB_ENGINE_FIREBIRD || !result) {
        return false;
    }

    (void)request;

    const char* desig = connection->designator ? connection->designator : SR_DATABASE;
    log_this(desig, "Firebird query execution not yet available (Phase 6)", LOG_LEVEL_ALERT, 0);

    *result = NULL;
    return false;
}

/*
 * Execute a prepared statement against a Firebird connection.
 * Phase 5: returns false, result is NULL. Phase 6 will implement.
 */
bool firebird_execute_prepared(DatabaseHandle* connection,
                               const PreparedStatement* stmt,
                               QueryRequest* request,
                               QueryResult** result) {
    if (!connection || connection->engine_type != DB_ENGINE_FIREBIRD || !stmt || !result) {
        return false;
    }

    (void)request;

    const char* desig = connection->designator ? connection->designator : SR_DATABASE;
    log_this(desig, "Firebird prepared statement execution not yet available (Phase 6)", LOG_LEVEL_ALERT, 0);

    *result = NULL;
    return false;
}
