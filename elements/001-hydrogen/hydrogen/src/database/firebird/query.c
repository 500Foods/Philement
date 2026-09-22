/*
 * Firebird Database Engine - Query Execution
 *
 * Implements firebird_execute_query / firebird_execute_prepared via
 * isc_dsql allocate / prepare / execute / fetch. Parity with other engines:
 * results returned as JSON array of row objects.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/database_params.h>

#include "types.h"
#include "connection.h"
#include "transaction.h"
#include "utils.h"
#include "query.h"

#include <ctype.h>

/* Fetch EOF from isc_dsql_fetch (same numeric value as historical SQLCODE 100). */
#define FB_FETCH_EOF 100

bool firebird_sql_expects_rows(const char* sql);

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
    return false;
}


/*
 * Minimal XSQLDA / XSQLVAR layouts (ibase.h not included in the build).
 * Sized for describe with up to FB_SQLDA_INIT_COLS columns, then grown.
 */
#define FB_SQLDA_INIT_COLS 20
#define FB_SQL_LONG 496
#define FB_SQL_SHORT 500
#define FB_SQL_INT64 580
#define FB_SQL_FLOAT 482
#define FB_SQL_DOUBLE 480
#define FB_SQL_VARYING 448
#define FB_SQL_TEXT 452
#define FB_SQL_BLOB 520

typedef struct {
    short sqltype;
    short sqlscale;
    short sqlsubtype;
    short sqllen;
    char* sqldata;
    short* sqlind;
    short sqlname_length;
    char  sqlname[32];
    short relname_length;
    char  relname[32];
    short ownname_length;
    char  ownname[32];
    short aliasname_length;
    char  aliasname[32];
} fb_xsqlvar_min;

typedef struct {
    short version;
    char  sqldaid[8];
    int   sqldabc;
    short sqln;
    short sqld;
    fb_xsqlvar_min sqlvar[1];
} fb_xsqlda_min;

fb_xsqlda_min* firebird_alloc_sqlda(short cols);
void firebird_free_sqlda_buffers(fb_xsqlda_min* sqlda);
bool firebird_bind_sqlda_buffers(fb_xsqlda_min* sqlda);
bool firebird_json_buffer_append(char** buf, size_t* size, size_t* cap, const char* piece);
bool firebird_json_append_escaped(char** buf, size_t* size, size_t* cap, const char* text);
void firebird_column_label_copy(const fb_xsqlvar_min* var, char* buf, size_t buflen);
bool firebird_append_cell_json(char** buf, size_t* size, size_t* cap, FirebirdConnection* fb_conn, const fb_xsqlvar_min* var);
char* firebird_read_blob_text(FirebirdConnection* fb_conn, const fb_quad_t* blob_id);


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

fb_xsqlda_min* firebird_alloc_sqlda(short cols) {
    size_t bytes = sizeof(fb_xsqlda_min) + (size_t)(cols > 0 ? cols - 1 : 0) * sizeof(fb_xsqlvar_min);
    fb_xsqlda_min* sqlda = calloc(1, bytes);
    if (!sqlda) {
        return NULL;
    }
    sqlda->version = 1;
    sqlda->sqln = cols;
    sqlda->sqld = 0;
    return sqlda;
}

void firebird_free_sqlda_buffers(fb_xsqlda_min* sqlda) {
    if (!sqlda) {
        return;
    }
    for (short i = 0; i < sqlda->sqln && i < sqlda->sqld; i++) {
        free(sqlda->sqlvar[i].sqldata);
        sqlda->sqlvar[i].sqldata = NULL;
        free(sqlda->sqlvar[i].sqlind);
        sqlda->sqlvar[i].sqlind = NULL;
    }
}

bool firebird_bind_sqlda_buffers(fb_xsqlda_min* sqlda) {
    if (!sqlda) {
        return false;
    }
    for (short i = 0; i < sqlda->sqld; i++) {
        short typ = (short)(sqlda->sqlvar[i].sqltype & ~1);
        short len = sqlda->sqlvar[i].sqllen;
        if (typ == FB_SQL_VARYING) {
            len = (short)(len + 2);
        } else if (typ == FB_SQL_BLOB) {
            len = (short)sizeof(fb_quad_t);
        }
        if (len < 1) {
            len = 64;
        }
        sqlda->sqlvar[i].sqldata = calloc(1, (size_t)len + 1);
        sqlda->sqlvar[i].sqlind = calloc(1, sizeof(short));
        if (!sqlda->sqlvar[i].sqldata || !sqlda->sqlvar[i].sqlind) {
            return false;
        }
    }
    return true;
}

bool firebird_json_buffer_append(char** buf, size_t* size, size_t* cap, const char* piece) {
    size_t piece_len = strlen(piece);
    if (*size + piece_len + 1 > *cap) {
        size_t new_cap = (*cap < 1024) ? 1024 : (*cap * 2);
        while (new_cap < *size + piece_len + 1) {
            new_cap *= 2;
        }
        char* nbuf = realloc(*buf, new_cap);
        if (!nbuf) {
            return false;
        }
        *buf = nbuf;
        *cap = new_cap;
    }
    memcpy(*buf + *size, piece, piece_len);
    *size += piece_len;
    (*buf)[*size] = '\0';
    return true;
}

bool firebird_json_append_escaped(char** buf, size_t* size, size_t* cap, const char* text) {
    if (!text) {
        return firebird_json_buffer_append(buf, size, cap, "null");
    }
    if (!firebird_json_buffer_append(buf, size, cap, "\"")) {
        return false;
    }
    for (const unsigned char* p = (const unsigned char*)text; *p; p++) {
        char tmp[8];
        if (*p == '"' || *p == '\\') {
            tmp[0] = '\\';
            tmp[1] = (char)*p;
            tmp[2] = '\0';
            if (!firebird_json_buffer_append(buf, size, cap, tmp)) {
                return false;
            }
        } else if (*p < 0x20 || *p >= 0x80) {
            snprintf(tmp, sizeof(tmp), "\\u%04x", *p);
            if (!firebird_json_buffer_append(buf, size, cap, tmp)) {
                return false;
            }
        } else {
            tmp[0] = (char)*p;
            tmp[1] = '\0';
            if (!firebird_json_buffer_append(buf, size, cap, tmp)) {
                return false;
            }
        }
    }
    return firebird_json_buffer_append(buf, size, cap, "\"");
}

void firebird_column_label_copy(const fb_xsqlvar_min* var, char* buf, size_t buflen) {
    if (!buf || buflen == 0) {
        return;
    }
    buf[0] = '\0';
    if (!var) {
        return;
    }
    const char* src = NULL;
    short len = 0;
    if (var->aliasname_length > 0) {
        src = var->aliasname;
        len = var->aliasname_length;
    } else if (var->sqlname_length > 0) {
        src = var->sqlname;
        len = var->sqlname_length;
    }
    if (!src || len <= 0) {
        snprintf(buf, buflen, "col");
        return;
    }
    if ((size_t)len >= buflen) {
        len = (short)(buflen - 1);
    }
    memcpy(buf, src, (size_t)len);
    buf[len] = '\0';
}


char* firebird_read_blob_text(FirebirdConnection* fb_conn, const fb_quad_t* blob_id) {
    if (!fb_conn || !fb_conn->db_handle || !fb_conn->tr_handle || !blob_id) {
        return NULL;
    }
    if (!isc_open_blob2_ptr || !isc_get_segment_ptr || !isc_close_blob_ptr) {
        return NULL;
    }

    fb_status_t status[FB_STATUS_LENGTH];
    void* blob_handle = NULL;
    fb_quad_t id = *blob_id;

    memset(status, 0, sizeof(status));
    fb_status_t rc = isc_open_blob2_ptr(
        status, &fb_conn->db_handle, &fb_conn->tr_handle,
        &blob_handle, &id, 0, NULL);
    if ((rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) || !blob_handle) {
        return NULL;
    }

    size_t cap = 4096;
    size_t len = 0;
    char* buf = malloc(cap);
    if (!buf) {
        memset(status, 0, sizeof(status));
        (void)isc_close_blob_ptr(status, &blob_handle);
        return NULL;
    }

    for (;;) {
        char segment[1024];
        unsigned short got = 0;
        memset(status, 0, sizeof(status));
        rc = isc_get_segment_ptr(status, &blob_handle, &got, (unsigned short)sizeof(segment), segment);
        fb_status_t code = status[1];
        if (got > 0) {
            if (len + got + 1 > cap) {
                size_t ncap = cap * 2;
                while (ncap < len + got + 1) {
                    ncap *= 2;
                }
                char* nbuf = realloc(buf, ncap);
                if (!nbuf) {
                    free(buf);
                    memset(status, 0, sizeof(status));
                    (void)isc_close_blob_ptr(status, &blob_handle);
                    return NULL;
                }
                buf = nbuf;
                cap = ncap;
            }
            memcpy(buf + len, segment, got);
            len += got;
        }
        if (code == FB_ISC_SEGSTR_EOF || rc == FB_ISC_SEGSTR_EOF) {
            break;
        }
        if (code != 0 && code != FB_ISC_SEGMENT && rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO && rc != FB_ISC_SEGMENT) {
            free(buf);
            memset(status, 0, sizeof(status));
            (void)isc_close_blob_ptr(status, &blob_handle);
            return NULL;
        }
        if (got == 0 && code == 0 && rc == 0) {
            break;
        }
    }

    memset(status, 0, sizeof(status));
    (void)isc_close_blob_ptr(status, &blob_handle);
    buf[len] = '\0';
    return buf;
}

bool firebird_append_cell_json(char** buf, size_t* size, size_t* cap, FirebirdConnection* fb_conn, const fb_xsqlvar_min* var) {
    if (var->sqlind && *var->sqlind < 0) {
        return firebird_json_buffer_append(buf, size, cap, "null");
    }
    short typ = (short)(var->sqltype & ~1);
    char numbuf[64];
    if (typ == FB_SQL_LONG && var->sqldata) {
        int v = 0;
        memcpy(&v, var->sqldata, sizeof(int) <= (size_t)var->sqllen ? sizeof(int) : (size_t)var->sqllen);
        snprintf(numbuf, sizeof(numbuf), "%d", v);
        return firebird_json_buffer_append(buf, size, cap, numbuf);
    }
    if (typ == FB_SQL_SHORT && var->sqldata) {
        short v = 0;
        memcpy(&v, var->sqldata, sizeof(short));
        snprintf(numbuf, sizeof(numbuf), "%d", (int)v);
        return firebird_json_buffer_append(buf, size, cap, numbuf);
    }
    if (typ == FB_SQL_INT64 && var->sqldata) {
        long long v = 0;
        memcpy(&v, var->sqldata, sizeof(long long) <= (size_t)var->sqllen ? sizeof(long long) : (size_t)var->sqllen);
        snprintf(numbuf, sizeof(numbuf), "%lld", v);
        return firebird_json_buffer_append(buf, size, cap, numbuf);
    }
    if ((typ == FB_SQL_FLOAT || typ == FB_SQL_DOUBLE) && var->sqldata) {
        double v = 0.0;
        if (typ == FB_SQL_FLOAT) {
            float f = 0.0f;
            memcpy(&f, var->sqldata, sizeof(float));
            v = (double)f;
        } else {
            memcpy(&v, var->sqldata, sizeof(double));
        }
        snprintf(numbuf, sizeof(numbuf), "%.15g", v);
        return firebird_json_buffer_append(buf, size, cap, numbuf);
    }
    if (typ == FB_SQL_VARYING && var->sqldata) {
        unsigned short vlen = 0;
        memcpy(&vlen, var->sqldata, sizeof(unsigned short));
        char* tmp = malloc((size_t)vlen + 1);
        if (!tmp) {
            return false;
        }
        memcpy(tmp, var->sqldata + 2, vlen);
        tmp[vlen] = '\0';
        bool ok = firebird_json_append_escaped(buf, size, cap, tmp);
        free(tmp);
        return ok;
    }
    if (typ == FB_SQL_TEXT && var->sqldata) {
        char* tmp = malloc((size_t)var->sqllen + 1);
        if (!tmp) {
            return false;
        }
        memcpy(tmp, var->sqldata, (size_t)var->sqllen);
        tmp[var->sqllen] = '\0';
        for (int i = var->sqllen - 1; i >= 0 && tmp[i] == ' '; i--) {
            tmp[i] = '\0';
        }
        bool ok = firebird_json_append_escaped(buf, size, cap, tmp);
        free(tmp);
        return ok;
    }
    if (typ == FB_SQL_BLOB && var->sqldata) {
        fb_quad_t id;
        memcpy(&id, var->sqldata, sizeof(id));
        char* text = firebird_read_blob_text(fb_conn, &id);
        if (!text) {
            return firebird_json_buffer_append(buf, size, cap, "null");
        }
        bool ok = firebird_json_append_escaped(buf, size, cap, text);
        free(text);
        return ok;
    }
    /* Fallback: do not treat opaque sqldata as a C string (blob ids are binary). */
    return firebird_json_buffer_append(buf, size, cap, "null");
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

    (void)parameters_json; /* named binds land in a follow-up; migrations use {} */

    bool own_txn = false;
    void* stmt_handle = NULL;
    fb_xsqlda_min* out_sqlda = NULL;
    bool buffers_owned = false;
    fb_status_t status[FB_STATUS_LENGTH];
    fb_status_t rc;
    QueryResult* db_result = calloc(1, sizeof(QueryResult));
    if (!db_result) {
        return false;
    }
    db_result->success = false;
    db_result->error_class = DB_ERR_OTHER;

    /* Ensure a transaction exists (migrations begin one; ad-hoc queries may not). */
    if (fb_conn->tr_handle == NULL) {
        if (!isc_start_transaction_ptr) {
            free(db_result);
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
            *result = firebird_build_error_result("Firebird failed to start transaction", DB_ERR_TRANSPORT);
            return false;
        }
        own_txn = true;
    }

    /* Non-row SQL (DDL/DML): execute_immediate — no statement/XSQLDA. */
    if (!firebird_sql_expects_rows(sql)) {
        if (!isc_dsql_execute_immediate_ptr) {
            if (own_txn && fb_conn->tr_handle && isc_rollback_transaction_ptr) {
                memset(status, 0, sizeof(status));
                (void)isc_rollback_transaction_ptr(status, &fb_conn->tr_handle);
                fb_conn->tr_handle = NULL;
            }
            free(db_result);
            *result = firebird_build_error_result("Firebird execute_immediate unavailable", DB_ERR_TRANSPORT);
            return false;
        }
        memset(status, 0, sizeof(status));
        rc = isc_dsql_execute_immediate_ptr(
            status, &fb_conn->db_handle, &fb_conn->tr_handle,
            0, sql, 3, NULL);
        if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
            firebird_status_to_error(status, desig);
            if (own_txn && fb_conn->tr_handle && isc_rollback_transaction_ptr) {
                memset(status, 0, sizeof(status));
                (void)isc_rollback_transaction_ptr(status, &fb_conn->tr_handle);
                fb_conn->tr_handle = NULL;
            }
            log_this(desig, "Firebird query failed SQL: %.200s%s", LOG_LEVEL_ERROR, 2,
                     sql, strlen(sql) > 200 ? "..." : "");
            db_result->error_class = DB_ERR_OTHER;
            db_result->error_message = strdup("Firebird query failed");
            db_result->data_json = strdup("[]");
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
                *result = firebird_build_error_result("Firebird commit failed after query", DB_ERR_TRANSPORT);
                return false;
            }
        }
        *result = db_result;
        return true;
    }

    if (!isc_dsql_allocate_ptr || !isc_dsql_prepare_ptr || !isc_dsql_execute_ptr ||
        !isc_dsql_fetch_ptr || !isc_dsql_free_statement_ptr) {
        if (own_txn && fb_conn->tr_handle && isc_rollback_transaction_ptr) {
            memset(status, 0, sizeof(status));
            (void)isc_rollback_transaction_ptr(status, &fb_conn->tr_handle);
            fb_conn->tr_handle = NULL;
        }
        free(db_result);
        *result = firebird_build_error_result("Firebird DSQL function pointers unavailable", DB_ERR_TRANSPORT);
        return false;
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
        0, sql, 3, out_sqlda);
    if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
        firebird_status_to_error(status, desig);
        goto fail_other;
    }

    /* Prepare fills descriptors only; never free engine-owned pointers. */
    for (short ci = 0; ci < out_sqlda->sqln; ci++) {
        out_sqlda->sqlvar[ci].sqldata = NULL;
        out_sqlda->sqlvar[ci].sqlind = NULL;
    }

    /* If prepare reported more columns than sqln, grow and re-describe via prepare again. */
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
            0, sql, 3, out_sqlda);
        if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
            firebird_status_to_error(status, desig);
            goto fail_other;
        }

        /* Prepare fills descriptors only; never free engine-owned pointers. */
        for (short ci = 0; ci < out_sqlda->sqln; ci++) {
            out_sqlda->sqlvar[ci].sqldata = NULL;
            out_sqlda->sqlvar[ci].sqlind = NULL;
        }
    }

    if (out_sqlda->sqld > 0) {
        if (!firebird_bind_sqlda_buffers(out_sqlda)) {
            goto fail_other;
        }
        buffers_owned = true;
    }

    memset(status, 0, sizeof(status));
    rc = isc_dsql_execute_ptr(status, &fb_conn->tr_handle, &stmt_handle, 1, NULL);
    if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
        firebird_status_to_error(status, desig);
        goto fail_other;
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
    if (out_sqlda->sqld > 0) {
        for (;;) {
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
    if (buffers_owned) {
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
            *result = firebird_build_error_result("Firebird commit failed after query", DB_ERR_TRANSPORT);
            return false;
        }
    }

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
    if (buffers_owned) {
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
             sql, strlen(sql) > 200 ? "..." : "");
    if (!db_result->error_message) {
        db_result->error_message = strdup("Firebird query failed");
    }
    if (!db_result->data_json) {
        db_result->data_json = strdup("[]");
    }
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
