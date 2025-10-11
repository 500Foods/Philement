/*
 * DB2 Database Engine - Interface Registration
 *
 * Implements DB2 engine interface registration.
 */

#include <src/hydrogen.h>
#include "../database.h"
#include "types.h"
#include "connection.h"
#include "query.h"
#include "transaction.h"
#include "prepared.h"
#include "utils.h"
#include "interface.h"

// Engine Interface Registration
static DatabaseEngineInterface db2_engine_interface = {
    .engine_type = DB_ENGINE_DB2,
    .name = (char*)"db2",
    .connect = db2_connect,
    .disconnect = db2_disconnect,
    .health_check = db2_health_check,
    .reset_connection = db2_reset_connection,
    .execute_query = db2_execute_query,
    .execute_prepared = db2_execute_prepared,
    .begin_transaction = db2_begin_transaction,
    .commit_transaction = db2_commit_transaction,
    .rollback_transaction = db2_rollback_transaction,
    .prepare_statement = db2_prepare_statement,
    .unprepare_statement = db2_unprepare_statement,
    .get_connection_string = db2_get_connection_string,
    .validate_connection_string = db2_validate_connection_string,
    .escape_string = db2_escape_string
};

DatabaseEngineInterface* db2_get_interface(void) {
    // Validate the interface structure before returning
    if (!db2_engine_interface.execute_query) {
        log_this(SR_DATABASE, "CRITICAL ERROR: DB2 engine interface execute_query is NULL!", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    if (!db2_engine_interface.name) {
        log_this(SR_DATABASE, "CRITICAL ERROR: DB2 engine interface name is NULL!", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // log_this(SR_DATABASE, "DB2 engine interface validated: name=%s, execute_query=%p", LOG_LEVEL_TRACE, 2,
    //     db2_engine_interface.name,
    //     (void*)(uintptr_t)db2_engine_interface.execute_query);

    return &db2_engine_interface;
}