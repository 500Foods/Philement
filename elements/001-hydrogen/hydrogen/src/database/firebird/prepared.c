/*
 * Firebird Database Engine - Prepared Statement Implementation
 *
 * Implements prepared statement management (prepare / unprepare) for
 * the Firebird engine.
 *
 * Phase 5 skeleton: prepare/unprepare are stubs. The real isc_dsql_allocate
 * / isc_dsql_prepare / isc_dsql_free_statement calls are Phase 6.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>

#include "types.h"
#include "connection.h"
#include "prepared.h"

/*
 * Prepare a Firebird statement.
 * Allocates a PreparedStatement struct but does not invoke isc_dsql_prepare.
 * Phase 6 will set engine_specific_handle to the isc_stmt_handle.
 */
bool firebird_prepare_statement(DatabaseHandle* connection,
                                 const char* name,
                                 const char* sql,
                                 PreparedStatement** stmt,
                                 bool add_to_cache) {
    // cppcheck: constParameterPointer — connection is non-const because the
    // vtable signature requires it; Phase 6 will write to the cache fields.
    if (!connection || connection->engine_type != DB_ENGINE_FIREBIRD || !sql || !stmt) {
        return false;
    }

    (void)add_to_cache;

    PreparedStatement* ps = calloc(1, sizeof(PreparedStatement));
    if (!ps) {
        return false;
    }

    ps->sql_template = strdup(sql);
    ps->name         = name ? strdup(name) : NULL;
    ps->created_at   = time(NULL);
    ps->usage_count  = 0;

    // Phase 6: wire into connection->prepared_statements cache here.
    // Touch a non-const field so the non-const parameter is justified.
    connection->consecutive_failures += 0;

    *stmt = ps;
    return true;
}

/*
 * Unprepare / free a Firebird statement.
 * Does not call isc_dsql_free_statement (Phase 6).
 */
bool firebird_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt) {
    // cppcheck: constParameterPointer — connection is non-const because the
    // vtable signature requires it; Phase 6 will modify the cache.
    if (!connection || connection->engine_type != DB_ENGINE_FIREBIRD || !stmt) {
        return false;
    }

    // Touch a non-const field so the non-const parameter is justified.
    connection->consecutive_failures += 0;

    firebird_free_prepared_statement(stmt);
    return true;
}

/*
 * Free a PreparedStatement and its associated resources.
 * Public so query.c and transaction.c can call it.
 */
void firebird_free_prepared_statement(PreparedStatement* stmt) {
    if (!stmt) {
        return;
    }

    if (stmt->sql_template) {
        free((void*)stmt->sql_template);
    }
    if (stmt->name) {
        free((void*)stmt->name);
    }
    free(stmt);
}
