/*
 * MySQL Database Engine - Interface Registration
 *
 * Implements MySQL engine interface registration.
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
static DatabaseEngineInterface mysql_engine_interface = {
    .engine_type = DB_ENGINE_MYSQL,
    .name = (char*)"mysql",
    .connect = mysql_connect,
    .disconnect = mysql_disconnect,
    .health_check = mysql_health_check,
    .reset_connection = mysql_reset_connection,
    .execute_query = mysql_execute_query,
    .execute_prepared = mysql_execute_prepared,
    .begin_transaction = mysql_begin_transaction,
    .commit_transaction = mysql_commit_transaction,
    .rollback_transaction = mysql_rollback_transaction,
    .prepare_statement = mysql_prepare_statement,
    .unprepare_statement = mysql_unprepare_statement,
    .get_connection_string = mysql_get_connection_string,
    .validate_connection_string = mysql_validate_connection_string,
    .escape_string = mysql_escape_string
};

DatabaseEngineInterface* mysql_get_interface(void) {
    return &mysql_engine_interface;
}