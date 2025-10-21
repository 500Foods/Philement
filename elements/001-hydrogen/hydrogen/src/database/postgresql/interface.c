/*
 * PostgreSQL Database Engine - Interface Registration
 *
 * Implements PostgreSQL engine interface registration.
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
static DatabaseEngineInterface postgresql_engine_interface = {
    .engine_type = DB_ENGINE_POSTGRESQL,
    .name = (char*)"postgresql",
    .connect = postgresql_connect,
    .disconnect = postgresql_disconnect,
    .health_check = postgresql_health_check,
    .reset_connection = postgresql_reset_connection,
    .execute_query = postgresql_execute_query,
    .execute_prepared = postgresql_execute_prepared,
    .begin_transaction = postgresql_begin_transaction,
    .commit_transaction = postgresql_commit_transaction,
    .rollback_transaction = postgresql_rollback_transaction,
    .prepare_statement = postgresql_prepare_statement,
    .unprepare_statement = postgresql_unprepare_statement,
    .get_connection_string = postgresql_get_connection_string,
    .validate_connection_string = postgresql_validate_connection_string,
    .escape_string = postgresql_escape_string
};

DatabaseEngineInterface* postgresql_get_interface(void) {
    // Validate the interface structure before returning
    if (!postgresql_engine_interface.execute_query) {
        log_this(SR_DATABASE, "CRITICAL ERROR: PostgreSQL engine interface execute_query is NULL!", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    if (!postgresql_engine_interface.name) {
        log_this(SR_DATABASE, "CRITICAL ERROR: PostgreSQL engine interface name is NULL!", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // log_this(SR_DATABASE, "PostgreSQL engine interface validated: name=%s, execute_query=%p", LOG_LEVEL_DEBUG, 2,
    //     postgresql_engine_interface.name,
    //     (void*)(uintptr_t)postgresql_engine_interface.execute_query);

    return &postgresql_engine_interface;
}