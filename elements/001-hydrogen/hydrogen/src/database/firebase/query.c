/*
 * Firebase engine - query execution. Phase 6 dispatches DDL and DML;
 * SELECT besides DROP_CHECK stays unimplemented.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include "query.h"
#include "sql_parse.h"
#include "sql_ddl.h"
#include "sql_dml.h"

QueryResult* firebase_query_result_error(const char* message) {
    QueryResult* error_result = calloc(1, sizeof(QueryResult));
    if (!error_result) {
        return NULL;
    }
    error_result->success = false;
    error_result->error_class = DB_ERR_OTHER;
    error_result->error_message = strdup(message ? message : "Firebase query failed");
    error_result->data_json = strdup("[]");
    if (!error_result->error_message || !error_result->data_json) {
        free(error_result->error_message);
        free(error_result->data_json);
        free(error_result);
        return NULL;
    }
    return error_result;
}

QueryResult* firebase_query_result_ok(int affected_rows) {
    QueryResult* ok = calloc(1, sizeof(QueryResult));
    if (!ok) {
        return NULL;
    }
    ok->success = true;
    ok->error_class = DB_ERR_NONE;
    ok->affected_rows = affected_rows;
    ok->data_json = strdup("[]");
    if (!ok->data_json) {
        free(ok);
        return NULL;
    }
    return ok;
}

QueryResult* firebase_build_not_implemented_result(const char* message) {
    return firebase_query_result_error(message ? message : "Firebase SQL is not implemented yet");
}

// cppcheck-suppress constParameterPointer
// Justification: Database engine interface requires non-const QueryRequest* parameter
bool firebase_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {
    if (!connection || !request || !result || connection->engine_type != DB_ENGINE_FIREBASE) {
        return false;
    }
    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    if (!request->sql_template) {
        *result = firebase_query_result_error("Firebase execute_query: SQL is NULL");
        return false;
    }

    FirebaseSqlStatement* stmt = firebase_sql_parse(request->sql_template);
    if (!stmt) {
        *result = firebase_query_result_error("Firebase execute_query: parse allocation failed");
        return false;
    }
    if (stmt->error_message) {
        log_this(designator, "Firebase SQL parse failed", LOG_LEVEL_ERROR, 0);
        *result = firebase_query_result_error(stmt->error_message);
        firebase_sql_statement_free(stmt);
        return false;
    }
    if (stmt->kind == FIREBASE_SQL_KIND_UNSUPPORTED || stmt->kind == FIREBASE_SQL_KIND_NONE) {
        log_this(designator, "Firebase execute_query: SQL interpreter not implemented", LOG_LEVEL_ERROR, 0);
        *result = firebase_build_not_implemented_result("Firebase SQL interpreter is not implemented yet");
        firebase_sql_statement_free(stmt);
        return false;
    }

    bool ok;
    if (firebase_sql_kind_is_dml(stmt->kind)) {
        ok = firebase_dml_execute(connection, stmt, result);
    } else {
        ok = firebase_ddl_execute(connection, stmt, result);
    }
    firebase_sql_statement_free(stmt);
    return ok;
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
