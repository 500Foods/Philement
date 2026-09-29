/*
 * MSSQL Database Engine - Interface Registration
 *
 * Implements MSSQL engine interface registration.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include "types.h"
#include "connection.h"
#include "query.h"
#include "transaction.h"
#include "prepared.h"
#include "utils.h"
#include "interface.h"

// Engine Interface Registration
static DatabaseEngineInterface mssql_engine_interface = {
    .engine_type = DB_ENGINE_MSSQL,
    .name = (char*)"mssql",
    .connect = mssql_connect,
    .disconnect = mssql_disconnect,
    .health_check = mssql_health_check,
    .reset_connection = mssql_reset_connection,
    .execute_query = mssql_execute_query,
    .execute_prepared = mssql_execute_prepared,
    .begin_transaction = mssql_begin_transaction,
    .commit_transaction = mssql_commit_transaction,
    .rollback_transaction = mssql_rollback_transaction,
    .prepare_statement = mssql_prepare_statement,
    .unprepare_statement = mssql_unprepare_statement,
    .get_connection_string = mssql_get_connection_string,
    .validate_connection_string = mssql_validate_connection_string,
    .escape_string = mssql_escape_string,
    .cancel_inflight = mssql_cancel_inflight
};

DatabaseEngineInterface* mssql_get_interface(void) {
    // Validate the interface structure before returning
    if (!mssql_engine_interface.execute_query) {
        log_this(SR_DATABASE, "CRITICAL ERROR: MSSQL engine interface execute_query is NULL!", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    if (!mssql_engine_interface.name) {
        log_this(SR_DATABASE, "CRITICAL ERROR: MSSQL engine interface name is NULL!", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    return &mssql_engine_interface;
}
