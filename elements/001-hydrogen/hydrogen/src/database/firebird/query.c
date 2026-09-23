/*
 * Firebird Database Engine - Query Execution
 *
 * Implements firebird_execute_query / firebird_execute_prepared via
 * isc_dsql allocate / prepare / execute / fetch. Parity with other engines:
 * results returned as JSON array of row objects.
 *
 * Row JSON, blobs, and XSQLDA buffers: query_result.c.
 * Parameter binding and the DATEADD rewrite: query_bind.c.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/database_params.h>

#include "types.h"
#include "connection.h"
#include "transaction.h"
#include "utils.h"
#include "query_internal.h"

#include <ctype.h>

/*
 * True when SQL is expected to produce a result set (prepare/fetch path).
 * DDL/DML without rows uses isc_dsql_execute_immediate — prepare+XSQLDA on
 * large CREATE OR ALTER FUNCTION bodies has corrupted the Firebird client heap.
 */
bool firebird_sql_expects_rows(const char* sql) {
    const char* p = sql;
    if (!p) {
        return false;
    }
    for (;;) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
            p++;
        }
        if (p[0] == '-' && p[1] == '-') {
            while (*p && *p != '\n') {
                p++;
            }
            continue;
        }
        if (p[0] == '/' && p[1] == '*') {
            p += 2;
            while (*p && !(p[0] == '*' && p[1] == '/')) {
                p++;
            }
            if (*p) {
                p += 2;
            }
            continue;
        }
        break;
    }
    if (strncasecmp(p, "SELECT", 6) == 0) {
        unsigned char c = (unsigned char)p[6];
        if (c == '\0' || (!isalnum(c) && c != '_')) {
            return true;
        }
    }
    if (strncasecmp(p, "WITH", 4) == 0) {
        unsigned char c = (unsigned char)p[4];
        if (c == '\0' || (!isalnum(c) && c != '_')) {
            return true;
        }
    }
    /* INSERT/UPDATE … RETURNING yields a result set (e.g. QueryRef #051). */
    {
        const char* s = p;
        while (*s) {
            if ((s[0] == 'R' || s[0] == 'r') &&
                strncasecmp(s, "RETURNING", 9) == 0) {
                unsigned char c = (unsigned char)s[9];
                if (c == '\0' || (!isalnum(c) && c != '_')) {
                    return true;
                }
            }
            s++;
        }
    }
    return false;
}

QueryResult* firebird_build_error_result(const char* error_msg, DatabaseErrorClass err_class) {
    QueryResult* db_result = calloc(1, sizeof(QueryResult));
    if (!db_result) {
        return NULL;
    }
    db_result->success = false;
    db_result->error_class = err_class;
    db_result->error_message = error_msg ? strdup(error_msg) : strdup("Firebird query failed");
    db_result->data_json = strdup("[]");
    db_result->row_count = 0;
    db_result->column_count = 0;
    db_result->affected_rows = -1;
    return db_result;
}


/*
 * isc_info_sql_stmt_type after prepare. Returns 0 when the info call is
 * unavailable so the caller can keep the SELECT/RETURNING text heuristic.
 * INSERT/UPDATE/DELETE … RETURNING that Firebird proves is a singleton is
 * FB_STMT_EXEC_PROCEDURE, not a selectable cursor.
 */
short firebird_statement_type(void* stmt_handle, const char* desig) {
    if (!stmt_handle || !isc_dsql_sql_info_ptr) {
        return 0;
    }

    char item = (char)FB_INFO_SQL_STMT_TYPE;
    char info[8];
    memset(info, 0, sizeof(info));

    fb_status_t status[FB_STATUS_LENGTH];
    memset(status, 0, sizeof(status));
    fb_status_t rc = isc_dsql_sql_info_ptr(
        status, &stmt_handle, 1, &item, (short)sizeof(info), info);
    if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
        firebird_status_to_error(status, desig);
        return 0;
    }
    if ((unsigned char)info[0] != (unsigned char)FB_INFO_SQL_STMT_TYPE) {
        return 0;
    }

    int len = (unsigned char)info[1] | ((unsigned char)info[2] << 8);
    /* info[8] is item + 2-byte length + at most 4 value bytes. */
    if (len < 1 || len > 4) {
        return 0;
    }

    int type = 0;
    for (int i = 0; i < len; i++) {
        type |= ((unsigned char)info[3 + i]) << (8 * i);
    }
    return (short)type;
}

/* Insert + update + delete counts from isc_info_sql_records. -1 if unavailable. */
int firebird_rows_affected(void* stmt_handle) {
    char item;
    char info[64];
    fb_status_t status[FB_STATUS_LENGTH];
    fb_status_t rc;
    int cluster;
    int pos;
    int end;
    int changed = 0;

    if (!stmt_handle || !isc_dsql_sql_info_ptr) {
        return -1;
    }
    item = (char)FB_INFO_SQL_RECORDS;
    memset(info, 0, sizeof(info));
    memset(status, 0, sizeof(status));
    rc = isc_dsql_sql_info_ptr(
        status, &stmt_handle, 1, &item, (short)sizeof(info), info);
    if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
        return -1;
    }
    if ((unsigned char)info[0] != (unsigned char)FB_INFO_SQL_RECORDS) {
        return -1;
    }
    cluster = (unsigned char)info[1] | ((unsigned char)info[2] << 8);
    if (cluster < 1) {
        return 0;
    }
    pos = 3;
    end = 3 + cluster;
    if (end > (int)sizeof(info)) {
        end = (int)sizeof(info);
    }
    while (pos < end) {
        unsigned char code;
        int vlen;
        int value = 0;
        int i;
        code = (unsigned char)info[pos];
        pos++;
        if (code == (unsigned char)FB_INFO_END) {
            break;
        }
        if (pos + 2 > end) {
            break;
        }
        vlen = (unsigned char)info[pos] | ((unsigned char)info[pos + 1] << 8);
        pos += 2;
        if (vlen < 1 || pos + vlen > end) {
            break;
        }
        for (i = 0; i < vlen && i < 4; i++) {
            value |= ((unsigned char)info[pos + i]) << (8 * i);
        }
        pos += vlen;
        if (code == (unsigned char)FB_INFO_REQ_INSERT_COUNT ||
            code == (unsigned char)FB_INFO_REQ_UPDATE_COUNT ||
            code == (unsigned char)FB_INFO_REQ_DELETE_COUNT) {
            changed += value;
        }
    }
    return changed;
}

bool firebird_execute_sql(DatabaseHandle* connection,
                          const char* sql,
                          const char* parameters_json,
                          QueryResult** result) {
    if (!connection || connection->engine_type != DB_ENGINE_FIREBIRD || !sql || !result) {
        return false;
    }

    const char* desig = firebird_designator_safe(connection);
    FirebirdConnection* fb_conn = (FirebirdConnection*)connection->connection_handle;
    if (!fb_conn || !fb_conn->db_handle) {
        *result = firebird_build_error_result("Invalid Firebird connection handle", DB_ERR_TRANSPORT);
        return false;
    }

    if (!isc_dsql_allocate_ptr || !isc_dsql_prepare_ptr || !isc_dsql_execute_ptr ||
        !isc_dsql_fetch_ptr || !isc_dsql_free_statement_ptr) {
        *result = firebird_build_error_result("Firebird DSQL function pointers unavailable", DB_ERR_TRANSPORT);
        return false;
    }

    ParameterList* param_list = NULL;
    TypedParameter** ordered_params = NULL;
    size_t ordered_count = 0;
    char* positional_sql = NULL;
    const char* sql_to_execute = sql;

    /* Empty / "{}" — leave SQL unchanged (migrations). */
    bool has_params = parameters_json && strlen(parameters_json) > 2;
    if (has_params) {
        param_list = parse_typed_parameters(parameters_json, desig);
        if (!param_list) {
            *result = firebird_build_error_result("Failed to parse Firebird parameters", DB_ERR_OTHER);
            return false;
        }
        positional_sql = convert_named_to_positional(
            sql, param_list, DB_ENGINE_FIREBIRD,
            &ordered_params, &ordered_count, desig);
        if (!positional_sql) {
            free_parameter_list(param_list);
            *result = firebird_build_error_result("Failed to convert Firebird named parameters", DB_ERR_OTHER);
            return false;
        }
        sql_to_execute = positional_sql;
    }

    /* Legacy DATEADD(+/- ? UNIT TO …) → DATEADD(UNIT, 0 +/- ?, …) for already-loaded QueryRefs. */
    {
        bool rewrite_oom = false;
        char* rewritten = firebird_rewrite_dateadd_params(sql_to_execute, &rewrite_oom);
        if (rewrite_oom) {
            free(positional_sql);
            free(ordered_params);
            free_parameter_list(param_list);
            *result = firebird_build_error_result("Out of memory rewriting Firebird DATEADD", DB_ERR_OTHER);
            return false;
        }
        if (rewritten) {
            free(positional_sql);
            positional_sql = rewritten;
            sql_to_execute = positional_sql;
        }
    }

    /* LENGTH() and untyped division, for QueryRefs already stored on disk. */
    {
        bool rewrite_oom = false;
        char* rewritten = firebird_rewrite_engine_sql(sql_to_execute, &rewrite_oom);
        if (rewrite_oom) {
            free(positional_sql);
            free(ordered_params);
            free_parameter_list(param_list);
            *result = firebird_build_error_result("Out of memory rewriting Firebird SQL", DB_ERR_OTHER);
            return false;
        }
        if (rewritten) {
            free(positional_sql);
            positional_sql = rewritten;
            sql_to_execute = positional_sql;
        }
    }

    bool own_txn = false;
    void* stmt_handle = NULL;
    fb_xsqlda_min* out_sqlda = NULL;
    fb_xsqlda_min* in_sqlda = NULL;
    bool out_buffers_owned = false;
    bool in_buffers_owned = false;
    fb_status_t status[FB_STATUS_LENGTH];
    fb_status_t rc;
    QueryResult* db_result = calloc(1, sizeof(QueryResult));
    if (!db_result) {
        free(positional_sql);
        free(ordered_params);
        free_parameter_list(param_list);
        return false;
    }
    db_result->success = false;
    db_result->error_class = DB_ERR_OTHER;

    /* Ensure a transaction exists (migrations begin one; ad-hoc queries may not). */
    if (fb_conn->tr_handle == NULL) {
        if (!isc_start_transaction_ptr) {
            free(db_result);
            free(positional_sql);
            free(ordered_params);
            free_parameter_list(param_list);
            *result = firebird_build_error_result("Firebird start_transaction unavailable", DB_ERR_TRANSPORT);
            return false;
        }
        const char* tpb = firebird_build_tpb(DB_ISOLATION_READ_COMMITTED);
        memset(status, 0, sizeof(status));
        rc = isc_start_transaction_ptr(
            status, &fb_conn->tr_handle, 1, &fb_conn->db_handle,
            (short)strlen(tpb), tpb);
        if ((rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) || !fb_conn->tr_handle) {
            firebird_status_to_error(status, desig);
            free(db_result);
            free(positional_sql);
            free(ordered_params);
            free_parameter_list(param_list);
            *result = firebird_build_error_result("Firebird failed to start transaction", DB_ERR_TRANSPORT);
            return false;
        }
        own_txn = true;
    }

    bool expects_rows = firebird_sql_expects_rows(sql_to_execute);
    /* Parameterized non-DDL uses prepare+execute; migrations keep execute_immediate. */
    bool use_prepare = expects_rows || ordered_count > 0;

    if (!use_prepare) {
        if (!isc_dsql_execute_immediate_ptr) {
            if (own_txn && fb_conn->tr_handle && isc_rollback_transaction_ptr) {
                memset(status, 0, sizeof(status));
                (void)isc_rollback_transaction_ptr(status, &fb_conn->tr_handle);
                fb_conn->tr_handle = NULL;
            }
            free(db_result);
            free(positional_sql);
            free(ordered_params);
            free_parameter_list(param_list);
            *result = firebird_build_error_result("Firebird execute_immediate unavailable", DB_ERR_TRANSPORT);
            return false;
        }
        memset(status, 0, sizeof(status));
        rc = isc_dsql_execute_immediate_ptr(
            status, &fb_conn->db_handle, &fb_conn->tr_handle,
            0, sql_to_execute, 3, NULL);
        if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
            firebird_status_to_error(status, desig);
            if (own_txn && fb_conn->tr_handle && isc_rollback_transaction_ptr) {
                memset(status, 0, sizeof(status));
                (void)isc_rollback_transaction_ptr(status, &fb_conn->tr_handle);
                fb_conn->tr_handle = NULL;
            }
            log_this(desig, "Firebird query failed SQL: %.200s%s", LOG_LEVEL_ERROR, 2,
                     sql_to_execute, strlen(sql_to_execute) > 200 ? "..." : "");
            db_result->error_class = DB_ERR_OTHER;
            db_result->error_message = strdup("Firebird query failed");
            db_result->data_json = strdup("[]");
            free(positional_sql);
            free(ordered_params);
            free_parameter_list(param_list);
            *result = db_result;
            return false;
        }
        db_result->success = true;
        db_result->error_class = DB_ERR_NONE;
        db_result->data_json = strdup("[]");
        db_result->row_count = 0;
        db_result->affected_rows = 0;
        if (own_txn && fb_conn->tr_handle && isc_commit_transaction_ptr) {
            memset(status, 0, sizeof(status));
            rc = isc_commit_transaction_ptr(status, &fb_conn->tr_handle);
            fb_conn->tr_handle = NULL;
            if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
                firebird_status_to_error(status, desig);
                database_engine_cleanup_result(db_result);
                free(positional_sql);
                free(ordered_params);
                free_parameter_list(param_list);
                *result = firebird_build_error_result("Firebird commit failed after query", DB_ERR_TRANSPORT);
                return false;
            }
        }
        free(positional_sql);
        free(ordered_params);
        free_parameter_list(param_list);
        *result = db_result;
        return true;
    }

    memset(status, 0, sizeof(status));
    rc = isc_dsql_allocate_ptr(status, &fb_conn->db_handle, &stmt_handle);
    if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
        firebird_status_to_error(status, desig);
        goto fail_transport;
    }
    firebird_active_stmt_set(connection, stmt_handle);

    out_sqlda = firebird_alloc_sqlda(FB_SQLDA_INIT_COLS);
    if (!out_sqlda) {
        goto fail_other;
    }

    memset(status, 0, sizeof(status));
    rc = isc_dsql_prepare_ptr(
        status, &fb_conn->tr_handle, &stmt_handle,
        0, sql_to_execute, 3, out_sqlda);
    if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
        firebird_status_to_error(status, desig);
        goto fail_other;
    }

    /* Prepare fills descriptors only; never free engine-owned pointers. */
    for (short ci = 0; ci < out_sqlda->sqln; ci++) {
        out_sqlda->sqlvar[ci].sqldata = NULL;
        out_sqlda->sqlvar[ci].sqlind = NULL;
    }

    if (out_sqlda->sqld > out_sqlda->sqln) {
        short need = out_sqlda->sqld;
        if (need > 512) {
            firebird_status_to_error(status, desig);
            goto fail_other;
        }
        firebird_free_sqlda_buffers(out_sqlda);
        free(out_sqlda);
        out_sqlda = firebird_alloc_sqlda(need);
        if (!out_sqlda) {
            goto fail_other;
        }
        memset(status, 0, sizeof(status));
        rc = isc_dsql_prepare_ptr(
            status, &fb_conn->tr_handle, &stmt_handle,
            0, sql_to_execute, 3, out_sqlda);
        if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
            firebird_status_to_error(status, desig);
            goto fail_other;
        }
        for (short ci = 0; ci < out_sqlda->sqln; ci++) {
            out_sqlda->sqlvar[ci].sqldata = NULL;
            out_sqlda->sqlvar[ci].sqlind = NULL;
        }
    }

    if (out_sqlda->sqld > 0) {
        if (!firebird_bind_sqlda_buffers(out_sqlda)) {
            goto fail_other;
        }
        out_buffers_owned = true;
    }

    if (ordered_count > 0) {
        if (!firebird_build_input_sqlda(&stmt_handle, ordered_params, ordered_count, &in_sqlda, desig)) {
            goto fail_other;
        }
        in_buffers_owned = (in_sqlda != NULL && in_sqlda->sqld > 0);
    }

    short stmt_type = firebird_statement_type(stmt_handle, desig);
    bool exec_proc = (stmt_type == FB_STMT_EXEC_PROCEDURE);
    /*
     * Selectable statements are fetched. A singleton RETURNING statement is
     * exec_procedure: isc_dsql_execute2 writes the one output row, and
     * isc_dsql_fetch on it returns SQLCODE -504 (cursor is not open).
     * stmt_type 0 keeps the previous text heuristic for mock builds.
     */
    bool fetch_rows = false;
    if (!exec_proc) {
        if (stmt_type == FB_STMT_SELECT || stmt_type == FB_STMT_SELECT_FOR_UPD) {
            fetch_rows = true;
        } else if (stmt_type == 0 && (expects_rows || (out_sqlda && out_sqlda->sqld > 0))) {
            fetch_rows = true;
        }
    }

    memset(status, 0, sizeof(status));
    if (exec_proc) {
        if (!isc_dsql_execute2_ptr) {
            db_result->error_message = strdup("Firebird execute2 unavailable");
            goto fail_transport;
        }
        const void* out_arg = (out_sqlda && out_sqlda->sqld > 0) ? (const void*)out_sqlda : NULL;
        rc = isc_dsql_execute2_ptr(
            status, &fb_conn->tr_handle, &stmt_handle, 1,
            in_sqlda ? (const void*)in_sqlda : NULL, out_arg);
        if (rc == FB_FETCH_EOF) {
            /* Singleton RETURNING matched zero rows. */
            exec_proc = false;
            rc = FB_SQL_SUCCESS;
        }
    } else {
        rc = isc_dsql_execute_ptr(status, &fb_conn->tr_handle, &stmt_handle, 1,
                                  in_sqlda ? (const void*)in_sqlda : NULL);
    }
    if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
        firebird_status_to_error(status, desig);
        goto fail_other;
    }

    bool one_row = exec_proc && out_sqlda && out_sqlda->sqld > 0;

    if (!fetch_rows && !one_row) {
        /* Parameterized DML with no output columns. */
        db_result->success = true;
        db_result->error_class = DB_ERR_NONE;
        db_result->data_json = strdup("[]");
        db_result->row_count = 0;
        {
            int changed_rows = firebird_rows_affected(stmt_handle);
            db_result->affected_rows = changed_rows < 0 ? 0 : changed_rows;
        }
        db_result->column_count = 0;

        firebird_active_stmt_clear(connection, stmt_handle);
        memset(status, 0, sizeof(status));
        (void)isc_dsql_free_statement_ptr(status, &stmt_handle, FB_DSQL_DEALLOCATE);
        stmt_handle = NULL;
        if (in_buffers_owned) {
            firebird_free_sqlda_buffers(in_sqlda);
        }
        free(in_sqlda);
        in_sqlda = NULL;
        if (out_buffers_owned) {
            firebird_free_sqlda_buffers(out_sqlda);
        }
        free(out_sqlda);
        out_sqlda = NULL;

        if (own_txn && fb_conn->tr_handle && isc_commit_transaction_ptr) {
            memset(status, 0, sizeof(status));
            rc = isc_commit_transaction_ptr(status, &fb_conn->tr_handle);
            fb_conn->tr_handle = NULL;
            if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
                firebird_status_to_error(status, desig);
                database_engine_cleanup_result(db_result);
                free(positional_sql);
                free(ordered_params);
                free_parameter_list(param_list);
                *result = firebird_build_error_result("Firebird commit failed after query", DB_ERR_TRANSPORT);
                return false;
            }
        }
        free(positional_sql);
        free(ordered_params);
        free_parameter_list(param_list);
        *result = db_result;
        return true;
    }

    db_result->column_count = (size_t)(out_sqlda->sqld > 0 ? out_sqlda->sqld : 0);
    if (db_result->column_count > 0) {
        db_result->column_names = calloc(db_result->column_count, sizeof(char*));
        if (!db_result->column_names) {
            goto fail_other;
        }
        for (size_t i = 0; i < db_result->column_count; i++) {
            char label[33];
            firebird_column_label_copy(&out_sqlda->sqlvar[i], label, sizeof(label));
            db_result->column_names[i] = strdup(label[0] ? label : "col");
            if (!db_result->column_names[i]) {
                goto fail_other;
            }
        }
    }

    size_t json_cap = 1024;
    size_t json_size = 0;
    char* json_buf = calloc(1, json_cap);
    if (!json_buf) {
        goto fail_other;
    }
    if (!firebird_json_buffer_append(&json_buf, &json_size, &json_cap, "[")) {
        free(json_buf);
        goto fail_other;
    }

    size_t row_count = 0;
    if (out_sqlda->sqld > 0 && (fetch_rows || one_row)) {
        for (;;) {
            if (!one_row) {
                memset(status, 0, sizeof(status));
                rc = isc_dsql_fetch_ptr(status, &stmt_handle, 1, out_sqlda);
                if (rc == FB_FETCH_EOF) {
                    break;
                }
                if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
                    firebird_status_to_error(status, desig);
                    free(json_buf);
                    goto fail_other;
                }
            }
            if (row_count > 0) {
                if (!firebird_json_buffer_append(&json_buf, &json_size, &json_cap, ",")) {
                    free(json_buf);
                    goto fail_other;
                }
            }
            if (!firebird_json_buffer_append(&json_buf, &json_size, &json_cap, "{")) {
                free(json_buf);
                goto fail_other;
            }
            for (size_t c = 0; c < db_result->column_count; c++) {
                if (c > 0) {
                    if (!firebird_json_buffer_append(&json_buf, &json_size, &json_cap, ",")) {
                        free(json_buf);
                        goto fail_other;
                    }
                }
                if (!firebird_json_append_escaped(&json_buf, &json_size, &json_cap, db_result->column_names[c])) {
                    free(json_buf);
                    goto fail_other;
                }
                if (!firebird_json_buffer_append(&json_buf, &json_size, &json_cap, ":")) {
                    free(json_buf);
                    goto fail_other;
                }
                if (!firebird_append_cell_json(&json_buf, &json_size, &json_cap, fb_conn, &out_sqlda->sqlvar[c])) {
                    free(json_buf);
                    goto fail_other;
                }
            }
            if (!firebird_json_buffer_append(&json_buf, &json_size, &json_cap, "}")) {
                free(json_buf);
                goto fail_other;
            }
            row_count++;
            if (one_row) {
                break;
            }
        }
    }

    if (!firebird_json_buffer_append(&json_buf, &json_size, &json_cap, "]")) {
        free(json_buf);
        goto fail_other;
    }

    db_result->data_json = json_buf;
    db_result->row_count = row_count;
    db_result->affected_rows = (out_sqlda->sqld > 0) ? (int)row_count : 0;
    db_result->success = true;
    db_result->error_class = DB_ERR_NONE;

    firebird_active_stmt_clear(connection, stmt_handle);
    memset(status, 0, sizeof(status));
    (void)isc_dsql_free_statement_ptr(status, &stmt_handle, FB_DSQL_DEALLOCATE);
    stmt_handle = NULL;
    if (in_buffers_owned) {
        firebird_free_sqlda_buffers(in_sqlda);
    }
    free(in_sqlda);
    in_sqlda = NULL;
    if (out_buffers_owned) {
        firebird_free_sqlda_buffers(out_sqlda);
    }
    free(out_sqlda);
    out_sqlda = NULL;

    if (own_txn && fb_conn->tr_handle && isc_commit_transaction_ptr) {
        memset(status, 0, sizeof(status));
        rc = isc_commit_transaction_ptr(status, &fb_conn->tr_handle);
        fb_conn->tr_handle = NULL;
        if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
            firebird_status_to_error(status, desig);
            database_engine_cleanup_result(db_result);
            free(positional_sql);
            free(ordered_params);
            free_parameter_list(param_list);
            *result = firebird_build_error_result("Firebird commit failed after query", DB_ERR_TRANSPORT);
            return false;
        }
    }

    free(positional_sql);
    free(ordered_params);
    free_parameter_list(param_list);
    *result = db_result;
    return true;

fail_transport:
    db_result->error_class = DB_ERR_TRANSPORT;
    goto fail_common;
fail_other:
    db_result->error_class = DB_ERR_OTHER;
fail_common:
    if (stmt_handle) {
        firebird_active_stmt_clear(connection, stmt_handle);
        memset(status, 0, sizeof(status));
        if (isc_dsql_free_statement_ptr) {
            (void)isc_dsql_free_statement_ptr(status, &stmt_handle, FB_DSQL_DEALLOCATE);
        }
    }
    if (in_buffers_owned) {
        firebird_free_sqlda_buffers(in_sqlda);
    }
    free(in_sqlda);
    in_sqlda = NULL;
    if (out_buffers_owned) {
        firebird_free_sqlda_buffers(out_sqlda);
    }
    free(out_sqlda);
    out_sqlda = NULL;
    if (own_txn && fb_conn->tr_handle && isc_rollback_transaction_ptr) {
        memset(status, 0, sizeof(status));
        (void)isc_rollback_transaction_ptr(status, &fb_conn->tr_handle);
        fb_conn->tr_handle = NULL;
    }
    if (db_result->column_names) {
        for (size_t i = 0; i < db_result->column_count; i++) {
            free(db_result->column_names[i]);
        }
        free(db_result->column_names);
        db_result->column_names = NULL;
    }
    log_this(desig, "Firebird query failed SQL: %.200s%s", LOG_LEVEL_ERROR, 2,
             sql_to_execute, strlen(sql_to_execute) > 200 ? "..." : "");
    if (!db_result->error_message) {
        db_result->error_message = strdup("Firebird query failed");
    }
    if (!db_result->data_json) {
        db_result->data_json = strdup("[]");
    }
    free(positional_sql);
    free(ordered_params);
    free_parameter_list(param_list);
    *result = db_result;
    return false;
}


// cppcheck-suppress constParameterPointer
// Justification: Database engine interface requires non-const QueryRequest* parameter
bool firebird_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {
    if (!connection || !request || !result || connection->engine_type != DB_ENGINE_FIREBIRD) {
        return false;
    }
    if (!request->sql_template) {
        *result = firebird_build_error_result("Missing SQL template", DB_ERR_OTHER);
        return false;
    }
    return firebird_execute_sql(connection, request->sql_template, request->parameters_json, result);
}

// cppcheck-suppress constParameterPointer
// Justification: Database engine interface requires non-const QueryRequest* parameter
bool firebird_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result) {
    if (!connection || !stmt || !result || connection->engine_type != DB_ENGINE_FIREBIRD) {
        return false;
    }
    const char* sql = stmt->sql_template;
    const char* params = request ? request->parameters_json : NULL;
    if (!sql) {
        *result = firebird_build_error_result("Prepared statement missing SQL", DB_ERR_OTHER);
        return false;
    }
    return firebird_execute_sql(connection, sql, params, result);
}
