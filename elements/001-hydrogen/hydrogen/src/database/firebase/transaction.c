/*
 * Firebase engine - REST transactions land in a later phase.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include "types.h"
#include "transaction.h"

bool firebase_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level,
                                Transaction** transaction) {
    (void)level;
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_FIREBASE) {
        return false;
    }
    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "Firebase begin_transaction: not implemented", LOG_LEVEL_ERROR, 0);
    return false;
}

// cppcheck-suppress constParameterPointer
// Justification: Database engine interface requires non-const Transaction* parameter
bool firebase_commit_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_FIREBASE) {
        return false;
    }
    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "Firebase commit_transaction: not implemented", LOG_LEVEL_ERROR, 0);
    return false;
}

// cppcheck-suppress constParameterPointer
// Justification: Database engine interface requires non-const Transaction* parameter
bool firebase_rollback_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction || connection->engine_type != DB_ENGINE_FIREBASE) {
        return false;
    }
    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "Firebase rollback_transaction: not implemented", LOG_LEVEL_ERROR, 0);
    return false;
}
