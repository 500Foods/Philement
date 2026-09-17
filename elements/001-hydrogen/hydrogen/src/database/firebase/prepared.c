/*
 * Firebase engine - prepared statements land with the SQL interpreter.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include "prepared.h"

bool firebase_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql,
                                PreparedStatement** stmt, bool add_to_cache) {
    (void)add_to_cache;
    if (!connection || !name || !sql || !stmt || connection->engine_type != DB_ENGINE_FIREBASE) {
        return false;
    }
    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "Firebase prepare_statement: not implemented", LOG_LEVEL_ERROR, 0);
    return false;
}

// cppcheck-suppress constParameterPointer
// Justification: Database engine interface requires non-const PreparedStatement* parameter
bool firebase_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt) {
    if (!connection || !stmt || connection->engine_type != DB_ENGINE_FIREBASE) {
        return false;
    }
    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "Firebase unprepare_statement: not implemented", LOG_LEVEL_ERROR, 0);
    return false;
}
