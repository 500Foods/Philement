/*
 * Firebase engine - interface registration.
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
#include "firebase.h"

static DatabaseEngineInterface firebase_engine_interface = {
    .engine_type = DB_ENGINE_FIREBASE,
    .name = (char*)"firebase",
    .connect = firebase_connect,
    .disconnect = firebase_disconnect,
    .health_check = firebase_health_check,
    .reset_connection = firebase_reset_connection,
    .execute_query = firebase_execute_query,
    .execute_prepared = firebase_execute_prepared,
    .begin_transaction = firebase_begin_transaction,
    .commit_transaction = firebase_commit_transaction,
    .rollback_transaction = firebase_rollback_transaction,
    .prepare_statement = firebase_prepare_statement,
    .unprepare_statement = firebase_unprepare_statement,
    .get_connection_string = firebase_get_connection_string,
    .validate_connection_string = firebase_validate_connection_string,
    .escape_string = firebase_escape_string,
    .cancel_inflight = firebase_cancel_inflight
};

DatabaseEngineInterface* firebase_get_interface(void) {
    /* Keep FB_* functions reachable until sql_expr (Phase 6) is the caller.
     * connect is never NULL, so this does not run. */
    if (!firebase_engine_interface.connect) {
        firebase_engine_test_functions();
    }
    return &firebase_engine_interface;
}
