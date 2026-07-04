/*
 * Database Engine Transaction Wrappers
 *
 * Thin passthroughs that look up the engine interface for the connection
 * and dispatch to the engine's begin/commit/rollback implementations.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>

bool database_engine_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction) {
    if (!connection || !transaction) {
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    DatabaseEngineInterface* engine = database_engine_get_with_designator(connection->engine_type, designator);
    if (!engine || !engine->begin_transaction) {
        return false;
    }

    return engine->begin_transaction(connection, level, transaction);
}

bool database_engine_commit_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction) {
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    DatabaseEngineInterface* engine = database_engine_get_with_designator(connection->engine_type, designator);
    if (!engine || !engine->commit_transaction) {
        return false;
    }

    return engine->commit_transaction(connection, transaction);
}

bool database_engine_rollback_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction) {
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    DatabaseEngineInterface* engine = database_engine_get_with_designator(connection->engine_type, designator);
    if (!engine || !engine->rollback_transaction) {
        return false;
    }

    return engine->rollback_transaction(connection, transaction);
}
