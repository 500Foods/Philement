/*
 * Firebird Database Engine - Interface Registration
 *
 * Implements Firebird engine interface registration. Mirrors db2/interface.c.
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

/*
 * ----------------------------------------------------------------------------
 * DatabaseEngineInterface vtable
 * ----------------------------------------------------------------------------
 */
static DatabaseEngineInterface firebird_engine_interface = {
    .engine_type            = DB_ENGINE_FIREBIRD,
    .name                   = (char*)"firebird",
    .connect                = firebird_connect,
    .disconnect             = firebird_disconnect,
    .health_check           = firebird_health_check,
    .reset_connection       = firebird_reset_connection,
    .execute_query          = firebird_execute_query,        // skeleton — Phase 6
    .execute_prepared       = firebird_execute_prepared,     // skeleton — Phase 6
    .begin_transaction      = firebird_begin_transaction,
    .commit_transaction     = firebird_commit_transaction,
    .rollback_transaction   = firebird_rollback_transaction,
    .prepare_statement      = firebird_prepare_statement,     // skeleton — Phase 6
    .unprepare_statement    = firebird_unprepare_statement,   // skeleton — Phase 6
    .get_connection_string  = firebird_get_connection_string,
    .validate_connection_string = firebird_validate_connection_string,
    .escape_string          = firebird_escape_string,
    .cancel_inflight        = firebird_cancel_inflight
};

DatabaseEngineInterface* firebird_get_interface(void) {
    // Validate the interface structure before returning
    if (!firebird_engine_interface.execute_query) {
        log_this(SR_DATABASE, "CRITICAL ERROR: Firebird engine interface execute_query is NULL!", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    if (!firebird_engine_interface.name) {
        log_this(SR_DATABASE, "CRITICAL ERROR: Firebird engine interface name is NULL!", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    return &firebird_engine_interface;
}
