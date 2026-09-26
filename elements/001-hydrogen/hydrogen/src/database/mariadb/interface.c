/*
 * MariaDB Database Engine - Interface Registration
 *
 * Implements MariaDB engine interface registration.
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
static DatabaseEngineInterface mariadb_engine_interface = {
    .engine_type = DB_ENGINE_MARIADB,
    .name = (char*)"mariadb",
    .connect = mariadb_connect,
    .disconnect = mariadb_disconnect,
    .health_check = mariadb_health_check,
    .reset_connection = mariadb_h_reset_connection,
    .execute_query = mariadb_execute_query,
    .execute_prepared = mariadb_execute_prepared,
    .begin_transaction = mariadb_begin_transaction,
    .commit_transaction = mariadb_commit_transaction,
    .rollback_transaction = mariadb_rollback_transaction,
    .prepare_statement = mariadb_prepare_statement,
    .unprepare_statement = mariadb_unprepare_statement,
    .get_connection_string = mariadb_get_connection_string,
    .validate_connection_string = mariadb_validate_connection_string,
    .escape_string = mariadb_h_escape_string,
    .cancel_inflight = mariadb_cancel_inflight
};

DatabaseEngineInterface* mariadb_get_interface(void) {
    return &mariadb_engine_interface;
}
