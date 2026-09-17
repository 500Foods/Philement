/*
 * Firebase engine - query execution. SQL interpreter lands in later phases.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include "query.h"

QueryResult* firebase_build_not_implemented_result(const char* message) {
    QueryResult* error_result = calloc(1, sizeof(QueryResult));
    if (!error_result) {
        return NULL;
    }
    error_result->success = false;
    error_result->error_class = DB_ERR_OTHER;
    error_result->error_message = strdup(message ? message : "Firebase SQL is not implemented yet");
    if (!error_result->error_message) {
        free(error_result);
        return NULL;
    }
    return error_result;
}

// cppcheck-suppress constParameterPointer
// Justification: Database engine interface requires non-const QueryRequest* parameter
bool firebase_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {
    if (!connection || !request || !result || connection->engine_type != DB_ENGINE_FIREBASE) {
        return false;
    }
    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "Firebase execute_query: SQL interpreter not implemented", LOG_LEVEL_ERROR, 0);
    *result = firebase_build_not_implemented_result("Firebase SQL interpreter is not implemented yet");
    return false;
}

// cppcheck-suppress constParameterPointer
// Justification: Database engine interface requires non-const QueryRequest* parameter
bool firebase_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result) {
    if (!connection || !stmt || !request || !result || connection->engine_type != DB_ENGINE_FIREBASE) {
        return false;
    }
    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "Firebase execute_prepared: SQL interpreter not implemented", LOG_LEVEL_ERROR, 0);
    *result = firebase_build_not_implemented_result("Firebase prepared statements are not implemented yet");
    return false;
}
