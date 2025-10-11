/*
 * SQLite Database Engine - Interface Registration
 *
 * Implements SQLite engine interface registration.
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
static DatabaseEngineInterface sqlite_engine_interface = {
    .engine_type = DB_ENGINE_SQLITE,
    .name = (char*)"sqlite",
    .connect = sqlite_connect,
    .disconnect = sqlite_disconnect,
    .health_check = sqlite_health_check,
    .reset_connection = sqlite_reset_connection,
    .execute_query = sqlite_execute_query,
    .execute_prepared = sqlite_execute_prepared,
    .begin_transaction = sqlite_begin_transaction,
    .commit_transaction = sqlite_commit_transaction,
    .rollback_transaction = sqlite_rollback_transaction,
    .prepare_statement = sqlite_prepare_statement,
    .unprepare_statement = sqlite_unprepare_statement,
    .get_connection_string = sqlite_get_connection_string,
    .validate_connection_string = sqlite_validate_connection_string,
    .escape_string = sqlite_escape_string
};

DatabaseEngineInterface* sqlite_get_interface(void) {
    // Validate the interface structure before returning
    if (!sqlite_engine_interface.execute_query) {
        log_this(SR_DATABASE, "CRITICAL ERROR: SQLite engine interface execute_query is NULL!", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    if (!sqlite_engine_interface.name) {
        log_this(SR_DATABASE, "CRITICAL ERROR: SQLite engine interface name is NULL!", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // log_this(SR_DATABASE, "SQLite engine interface validated: name=%s, execute_query=%p", LOG_LEVEL_DEBUG, 2,
    //     sqlite_engine_interface.name,
    //     (void*)(uintptr_t)sqlite_engine_interface.execute_query);

    return &sqlite_engine_interface;
}