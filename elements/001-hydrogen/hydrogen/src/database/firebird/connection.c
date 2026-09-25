/*
 * Firebird Database Engine - Connection Management Implementation
 *
 * Implements Firebird connection management via libfbclient (isc_* API).
 * Function pointers are loaded via dlopen at runtime, or mocked in
 * Unity tests via USE_MOCK_LIBFBC.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>

// Local includes
#include "types.h"
#include "connection.h"
#include "transaction.h"
#include "utils.h"

/*
 * ----------------------------------------------------------------------------
 * isc_* function pointer definitions
 *
 * In production these are loaded from libfbclient via dlopen in
 * load_libfbclient_functions(). In Unity tests, USE_MOCK_LIBFBC remaps
 * them to mock_isc_* functions declared in mock_libfbclient.h.
 * ----------------------------------------------------------------------------
 */
#ifdef USE_MOCK_LIBFBC
#include <unity/mocks/mock_libfbclient.h>
#endif

isc_attach_database_t          isc_attach_database_ptr          = NULL;
isc_detach_database_t          isc_detach_database_ptr          = NULL;
isc_start_transaction_t        isc_start_transaction_ptr        = NULL;
isc_commit_transaction_t       isc_commit_transaction_ptr       = NULL;
isc_rollback_transaction_t     isc_rollback_transaction_ptr    = NULL;
isc_dsql_allocate_t            isc_dsql_allocate_ptr           = NULL;
isc_dsql_prepare_t             isc_dsql_prepare_ptr            = NULL;
isc_dsql_execute_t             isc_dsql_execute_ptr            = NULL;
isc_dsql_execute2_t            isc_dsql_execute2_ptr           = NULL;
isc_dsql_execute_immediate_t   isc_dsql_execute_immediate_ptr  = NULL;
isc_dsql_free_statement_t      isc_dsql_free_statement_ptr     = NULL;
isc_dsql_fetch_t               isc_dsql_fetch_ptr              = NULL;
isc_dsql_sql_info_t            isc_dsql_sql_info_ptr           = NULL;
isc_dsql_describe_bind_t       isc_dsql_describe_bind_ptr      = NULL;
isc_decode_sql_date_t          isc_decode_sql_date_ptr         = NULL;
isc_decode_sql_time_t          isc_decode_sql_time_ptr         = NULL;
isc_decode_timestamp_t         isc_decode_timestamp_ptr        = NULL;
isc_encode_sql_date_t          isc_encode_sql_date_ptr         = NULL;
isc_encode_sql_time_t          isc_encode_sql_time_ptr         = NULL;
isc_encode_timestamp_t         isc_encode_timestamp_ptr        = NULL;
fb_cancel_operation_t          fb_cancel_operation_ptr         = NULL;
isc_open_blob2_t               isc_open_blob2_ptr              = NULL;
isc_get_segment_t              isc_get_segment_ptr             = NULL;
isc_close_blob_t               isc_close_blob_ptr              = NULL;
fb_interpret_t                 fb_interpret_ptr                = NULL;

#ifndef USE_MOCK_LIBFBC
// Library handle for dynamic loading (only in production builds)
static void* libfbclient_handle = NULL;
static pthread_mutex_t libfbclient_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/* One isc_attach_database at a time. libChaCha's TomCrypt setup is not thread-safe. */
static pthread_mutex_t firebird_attach_mutex = PTHREAD_MUTEX_INITIALIZER;
static int firebird_wire_crypt_loaded = 0;

void firebird_preload_wire_crypt(void) {
    pthread_mutex_lock(&firebird_attach_mutex);
    if (firebird_wire_crypt_loaded) {
        pthread_mutex_unlock(&firebird_attach_mutex);
        return;
    }

    const char* fb_root = getenv("FIREBIRD");
    char from_env[512];
    const char* paths[3];
    int npaths = 0;
    if (fb_root && fb_root[0] != '\0') {
        snprintf(from_env, sizeof(from_env), "%s/plugins/libChaCha.so", fb_root);
        paths[npaths++] = from_env;
    }
    paths[npaths++] = "/usr/lib64/firebird/plugins/libChaCha.so";
    paths[npaths++] = "/usr/lib/firebird/plugins/libChaCha.so";

    for (int i = 0; i < npaths; i++) {
        /* RTLD_NOW binds sha256_init to libtomcrypt immediately.
         * Do not dlclose: a later load would rebind against crypto.so. */
        void* handle = dlopen(paths[i], RTLD_NOW | RTLD_LOCAL);
        if (handle) {
            firebird_wire_crypt_loaded = 1;
            break;
        }
    }
    pthread_mutex_unlock(&firebird_attach_mutex);
}

/*
 * ----------------------------------------------------------------------------
 * Library loading
 * ----------------------------------------------------------------------------
 */
bool load_libfbclient_functions(const char* designator) {
#ifdef USE_MOCK_LIBFBC
    (void)designator;
    // In mock mode, assign the _ptr variables to the mock_* functions
    // so that connection.c / query.c / transaction.c use the mocks.
    isc_attach_database_ptr       = mock_isc_attach_database;
    isc_detach_database_ptr       = mock_isc_detach_database;
    isc_start_transaction_ptr     = mock_isc_start_transaction;
    isc_commit_transaction_ptr    = mock_isc_commit_transaction;
    isc_rollback_transaction_ptr  = mock_isc_rollback_transaction;
    isc_dsql_allocate_ptr         = mock_isc_dsql_allocate;
    isc_dsql_prepare_ptr          = mock_isc_dsql_prepare;
    isc_dsql_execute_ptr          = mock_isc_dsql_execute;
    isc_dsql_execute2_ptr         = mock_isc_dsql_execute2;
    isc_dsql_execute_immediate_ptr = mock_isc_dsql_execute_immediate;
    isc_dsql_free_statement_ptr   = mock_isc_dsql_free_statement;
    isc_dsql_fetch_ptr            = mock_isc_dsql_fetch;
    isc_dsql_sql_info_ptr         = mock_isc_dsql_sql_info;
    isc_dsql_describe_bind_ptr    = mock_isc_dsql_describe_bind;
    isc_decode_sql_date_ptr       = mock_isc_decode_sql_date;
    isc_decode_sql_time_ptr       = mock_isc_decode_sql_time;
    isc_decode_timestamp_ptr      = mock_isc_decode_timestamp;
    isc_encode_sql_date_ptr       = mock_isc_encode_sql_date;
    isc_encode_sql_time_ptr       = mock_isc_encode_sql_time;
    isc_encode_timestamp_ptr      = mock_isc_encode_timestamp;
    fb_cancel_operation_ptr       = mock_fb_cancel_operation;
    isc_open_blob2_ptr            = mock_isc_open_blob2;
    isc_get_segment_ptr           = mock_isc_get_segment;
    isc_close_blob_ptr            = mock_isc_close_blob;
    fb_interpret_ptr              = mock_fb_interpret;
    return true;
#else
    const char* log_subsystem = designator ? designator : SR_DATABASE;

    MUTEX_LOCK(&libfbclient_mutex, log_subsystem);

    if (libfbclient_handle) {
        MUTEX_UNLOCK(&libfbclient_mutex, log_subsystem);
        return true;
    }

    static const char* names[] = {
        "libfbclient.so.2",
        "libfbclient.so",
        "libfbclient.so.40",
        "libfbclient.so.30",
        NULL
    };

    for (int i = 0; names[i]; i++) {
        libfbclient_handle = dlopen(names[i], RTLD_LAZY);
        if (libfbclient_handle) {
            break;
        }
    }

    if (!libfbclient_handle) {
        MUTEX_UNLOCK(&libfbclient_mutex, log_subsystem);
        return false;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    isc_attach_database_ptr       = (isc_attach_database_t)          dlsym(libfbclient_handle, "isc_attach_database");
    isc_detach_database_ptr       = (isc_detach_database_t)          dlsym(libfbclient_handle, "isc_detach_database");
    isc_start_transaction_ptr     = (isc_start_transaction_t)         dlsym(libfbclient_handle, "isc_start_transaction");
    isc_commit_transaction_ptr    = (isc_commit_transaction_t)        dlsym(libfbclient_handle, "isc_commit_transaction");
    isc_rollback_transaction_ptr  = (isc_rollback_transaction_t)      dlsym(libfbclient_handle, "isc_rollback_transaction");
    isc_dsql_allocate_ptr         = (isc_dsql_allocate_t)             dlsym(libfbclient_handle, "isc_dsql_allocate_statement");
    if (!isc_dsql_allocate_ptr) {
        isc_dsql_allocate_ptr     = (isc_dsql_allocate_t)             dlsym(libfbclient_handle, "isc_dsql_alloc_statement2");
    }
    isc_dsql_prepare_ptr          = (isc_dsql_prepare_t)              dlsym(libfbclient_handle, "isc_dsql_prepare");
    isc_dsql_execute_ptr          = (isc_dsql_execute_t)              dlsym(libfbclient_handle, "isc_dsql_execute");
    isc_dsql_execute2_ptr         = (isc_dsql_execute2_t)             dlsym(libfbclient_handle, "isc_dsql_execute2");
    isc_dsql_execute_immediate_ptr= (isc_dsql_execute_immediate_t)    dlsym(libfbclient_handle, "isc_dsql_execute_immediate");
    isc_dsql_free_statement_ptr   = (isc_dsql_free_statement_t)       dlsym(libfbclient_handle, "isc_dsql_free_statement");
    isc_dsql_fetch_ptr            = (isc_dsql_fetch_t)                dlsym(libfbclient_handle, "isc_dsql_fetch");
    isc_dsql_sql_info_ptr         = (isc_dsql_sql_info_t)             dlsym(libfbclient_handle, "isc_dsql_sql_info");
    isc_dsql_describe_bind_ptr    = (isc_dsql_describe_bind_t)        dlsym(libfbclient_handle, "isc_dsql_describe_bind");
    isc_decode_sql_date_ptr       = (isc_decode_sql_date_t)           dlsym(libfbclient_handle, "isc_decode_sql_date");
    isc_decode_sql_time_ptr       = (isc_decode_sql_time_t)           dlsym(libfbclient_handle, "isc_decode_sql_time");
    isc_decode_timestamp_ptr      = (isc_decode_timestamp_t)          dlsym(libfbclient_handle, "isc_decode_timestamp");
    isc_encode_sql_date_ptr       = (isc_encode_sql_date_t)           dlsym(libfbclient_handle, "isc_encode_sql_date");
    isc_encode_sql_time_ptr       = (isc_encode_sql_time_t)           dlsym(libfbclient_handle, "isc_encode_sql_time");
    isc_encode_timestamp_ptr      = (isc_encode_timestamp_t)          dlsym(libfbclient_handle, "isc_encode_timestamp");
    fb_cancel_operation_ptr       = (fb_cancel_operation_t)           dlsym(libfbclient_handle, "fb_cancel_operation");
    isc_open_blob2_ptr            = (isc_open_blob2_t)                dlsym(libfbclient_handle, "isc_open_blob2");
    isc_get_segment_ptr           = (isc_get_segment_t)               dlsym(libfbclient_handle, "isc_get_segment");
    isc_close_blob_ptr            = (isc_close_blob_t)                dlsym(libfbclient_handle, "isc_close_blob");
    fb_interpret_ptr              = (fb_interpret_t)                  dlsym(libfbclient_handle, "fb_interpret");
#pragma GCC diagnostic pop

    MUTEX_UNLOCK(&libfbclient_mutex, log_subsystem);
    return true;
#endif
}

/*
 * ----------------------------------------------------------------------------
 * FirebirdConnection wrapper management
 * ----------------------------------------------------------------------------
 */
FirebirdConnection* firebird_create_connection_wrapper(void) {
    FirebirdConnection* fb_conn = calloc(1, sizeof(FirebirdConnection));
    if (!fb_conn) {
        return NULL;
    }
    fb_conn->db_handle   = NULL;
    fb_conn->tr_handle   = NULL;
    fb_conn->stmt_handle = NULL;
    pthread_mutex_init(&fb_conn->stmt_lock, NULL);
    return fb_conn;
}

void firebird_destroy_connection_wrapper(FirebirdConnection* fb_conn) {
    if (!fb_conn) {
        return;
    }
    pthread_mutex_destroy(&fb_conn->stmt_lock);
    free(fb_conn);
}

FirebirdConnection* firebird_get_connection_wrapper(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_FIREBIRD) {
        return NULL;
    }
    return (FirebirdConnection*)connection->connection_handle;
}

/*
 * ----------------------------------------------------------------------------
 * Forward declaration for designator helper
 * ----------------------------------------------------------------------------
 */

/*
 * ----------------------------------------------------------------------------
 * Connection management
 * ----------------------------------------------------------------------------
 */
bool firebird_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator) {
    const char* log_subsystem = designator ? designator : SR_DATABASE;

    if (!config || !connection) {
        log_this(log_subsystem, "Invalid parameters for Firebird connection", LOG_LEVEL_ERROR, 0);
        return false;
    }

    if (!load_libfbclient_functions(designator)) {
        log_this(log_subsystem, "Firebird connection failed: libfbclient not available", LOG_LEVEL_ERROR, 0);
        return false;
    }
    firebird_preload_wire_crypt();

    FirebirdConnection* fb_conn = firebird_create_connection_wrapper();
    if (!fb_conn) {
        log_this(log_subsystem, "Firebird connection failed: wrapper allocation failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    size_t dpb_len = 0;
    char* attach_params = firebird_build_attach_string(config, &dpb_len);
    if (!attach_params) {
        log_this(log_subsystem, "Firebird connection failed: attach string allocation failed", LOG_LEVEL_ERROR, 0);
        firebird_destroy_connection_wrapper(fb_conn);
        return false;
    }

    // Build the dbname string. Firebird isc_attach_database expects:
    //   - Embedded mode: a bare filesystem path like "/var/lib/.../test.fdb"
    //   - Network mode:  "host:/path/to/db.fdb" or "host/port:/path/to/db.fdb"
    //    (the separator between host and port is '/', not ':')
    // Note: db_path may already start with '/' (absolute path), so we must not
    // prepend another '/' — that would produce 'host/port://path' (double slash).
    const char* db_path = config->database ? config->database : "";
    bool db_path_is_absolute = (db_path[0] == '/');
    char* db_name = NULL;

    if (config->host && *config->host &&
        strcmp(config->host, "localhost") != 0 &&
        strcmp(config->host, "127.0.0.1") != 0) {
        // Remote host — build "host/port:/path" or "host:/path"
        size_t host_len = strlen(config->host);
        size_t path_len = strlen(db_path);
        db_name = malloc(host_len + path_len + 32);
        if (!db_name) {
            free(attach_params);
            firebird_destroy_connection_wrapper(fb_conn);
            return false;
        }
        if (config->port > 0) {
            if (db_path_is_absolute) {
                snprintf(db_name, host_len + path_len + 32, "%s/%d:%s", config->host, config->port, db_path);
            } else {
                snprintf(db_name, host_len + path_len + 32, "%s/%d:/%s", config->host, config->port, db_path);
            }
        } else {
            if (db_path_is_absolute) {
                snprintf(db_name, host_len + path_len + 32, "%s:%s", config->host, db_path);
            } else {
                snprintf(db_name, host_len + path_len + 32, "%s:/%s", config->host, db_path);
            }
        }
    } else if (config->host && *config->host &&
               (strcmp(config->host, "localhost") == 0 ||
                strcmp(config->host, "127.0.0.1") == 0)) {
        // Local network connection — use "host/path:/path" format
        size_t host_len = strlen(config->host);
        size_t path_len = strlen(db_path);
        db_name = malloc(host_len + path_len + 32);
        if (!db_name) {
            free(attach_params);
            firebird_destroy_connection_wrapper(fb_conn);
            return false;
        }
        if (config->port > 0) {
            if (db_path_is_absolute) {
                snprintf(db_name, host_len + path_len + 32, "%s/%d:%s", config->host, config->port, db_path);
            } else {
                snprintf(db_name, host_len + path_len + 32, "%s/%d:/%s", config->host, config->port, db_path);
            }
        } else {
            if (db_path_is_absolute) {
                snprintf(db_name, host_len + path_len + 32, "%s:%s", config->host, db_path);
            } else {
                snprintf(db_name, host_len + path_len + 32, "%s:/%s", config->host, db_path);
            }
        }
    } else {
        // Embedded mode — just the bare path
        db_name = strdup(db_path);
        if (!db_name) {
            free(attach_params);
            firebird_destroy_connection_wrapper(fb_conn);
            return false;
        }
    }

    fb_status_t status[FB_STATUS_LENGTH];
    memset(status, 0, sizeof(status));

    pthread_mutex_lock(&firebird_attach_mutex);
    fb_status_t result = isc_attach_database_ptr(
        status,
        (short)(db_name ? strlen(db_name) : 0),
        db_name,
        &fb_conn->db_handle,
        (short)dpb_len,
        attach_params
    );
    pthread_mutex_unlock(&firebird_attach_mutex);

    free(attach_params);
    free(db_name);

    if (result != FB_SQL_SUCCESS && result != FB_SQL_SUCCESS_INFO) {
        firebird_status_to_error(status, log_subsystem);
        firebird_destroy_connection_wrapper(fb_conn);
        return false;
    }

    DatabaseHandle* db_handle = calloc(1, sizeof(DatabaseHandle));
    if (!db_handle) {
        if (isc_detach_database_ptr) {
            isc_detach_database_ptr(status, &fb_conn->db_handle);
        }
        firebird_destroy_connection_wrapper(fb_conn);
        return false;
    }

    db_handle->engine_type        = DB_ENGINE_FIREBIRD;
    db_handle->connection_handle  = fb_conn;
    db_handle->config             = config;
    db_handle->status             = DB_CONNECTION_CONNECTED;
    db_handle->connected_since    = time(NULL);
    db_handle->current_transaction = NULL;
    db_handle->designator         = designator ? strdup(designator) : NULL;
    db_handle->last_health_check  = 0;
    db_handle->consecutive_failures = 0;
    pthread_mutex_init(&db_handle->connection_lock, NULL);

    *connection = db_handle;
    return true;
}

bool firebird_disconnect(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_FIREBIRD) {
        return false;
    }

    FirebirdConnection* fb_conn = (FirebirdConnection*)connection->connection_handle;
    if (fb_conn) {
        if (fb_conn->tr_handle && isc_rollback_transaction_ptr) {
            fb_status_t status[FB_STATUS_LENGTH];
            memset(status, 0, sizeof(status));
            (void)isc_rollback_transaction_ptr(status, &fb_conn->tr_handle);
        }
        if (fb_conn->db_handle && isc_detach_database_ptr) {
            fb_status_t status[FB_STATUS_LENGTH];
            memset(status, 0, sizeof(status));
            isc_detach_database_ptr(status, &fb_conn->db_handle);
        }
        firebird_destroy_connection_wrapper(fb_conn);
    }

    connection->status = DB_CONNECTION_DISCONNECTED;
    log_this(firebird_designator_safe(connection), "Firebird connection closed", LOG_LEVEL_TRACE, 0);

    return true;
}

bool firebird_health_check(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_FIREBIRD) {
        return false;
    }

    FirebirdConnection* fb_conn = (FirebirdConnection*)connection->connection_handle;
    if (!fb_conn || !fb_conn->db_handle) {
        return false;
    }

    const char* desig = firebird_designator_safe(connection);
    fb_status_t status[FB_STATUS_LENGTH];
    bool own_health_txn = false;
    void* stmt_handle = NULL;
    bool ok = false;

    if (!isc_dsql_allocate_ptr || !isc_dsql_prepare_ptr || !isc_dsql_execute_ptr ||
        !isc_dsql_fetch_ptr || !isc_dsql_free_statement_ptr) {
        connection->consecutive_failures++;
        log_this(desig, "Firebird health check failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    memset(status, 0, sizeof(status));

    /* Firebird requires an active transaction for DSQL execution. */
    if (fb_conn->tr_handle == NULL && isc_start_transaction_ptr) {
        const char* tpb = firebird_build_tpb(DB_ISOLATION_READ_COMMITTED);
        fb_status_t tr_result = isc_start_transaction_ptr(
            status,
            &fb_conn->tr_handle,
            1,
            &fb_conn->db_handle,
            (short)strlen(tpb),
            tpb
        );
        if (tr_result != FB_SQL_SUCCESS && tr_result != FB_SQL_SUCCESS_INFO) {
            firebird_status_to_error(status, desig);
            connection->consecutive_failures++;
            log_this(desig, "Firebird health check failed", LOG_LEVEL_ERROR, 0);
            return false;
        }
        own_health_txn = true;
    }

    if (fb_conn->tr_handle == NULL) {
        connection->consecutive_failures++;
        log_this(desig, "Firebird health check failed", LOG_LEVEL_ERROR, 0);
        return false;
    }

    /*
     * Minimal XSQLDA version 1 for one integer output column.
     * ibase.h is not included in the Hydrogen build; keep layout compatible.
     */
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

    fb_xsqlda_min out_sqlda;
    int out_value = 0;
    short out_ind = 0;
    const char* health_sql = "SELECT 1 FROM RDB$DATABASE";
    fb_status_t rc;

    memset(status, 0, sizeof(status));
    rc = isc_dsql_allocate_ptr(status, &fb_conn->db_handle, &stmt_handle);
    if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
        firebird_status_to_error(status, desig);
        goto health_done;
    }

    memset(&out_sqlda, 0, sizeof(out_sqlda));
    out_sqlda.version = 1; /* SQLDA_VERSION1 */
    out_sqlda.sqln = 1;
    out_sqlda.sqld = 1;
    out_sqlda.sqlvar[0].sqltype = 496; /* SQL_LONG */
    out_sqlda.sqlvar[0].sqllen = (short)sizeof(int);
    out_sqlda.sqlvar[0].sqldata = (char*)&out_value;
    out_sqlda.sqlvar[0].sqlind = &out_ind;

    memset(status, 0, sizeof(status));
    rc = isc_dsql_prepare_ptr(
        status,
        &fb_conn->tr_handle,
        &stmt_handle,
        (short)strlen(health_sql),
        health_sql,
        3,
        &out_sqlda
    );
    if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
        firebird_status_to_error(status, desig);
        goto health_done;
    }

    /* Prepare may rewrite type/length; re-bind output buffers. */
    out_sqlda.sqlvar[0].sqldata = (char*)&out_value;
    out_sqlda.sqlvar[0].sqlind = &out_ind;

    memset(status, 0, sizeof(status));
    rc = isc_dsql_execute_ptr(
        status,
        &fb_conn->tr_handle,
        &stmt_handle,
        1,
        NULL
    );
    if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
        firebird_status_to_error(status, desig);
        goto health_done;
    }

    memset(status, 0, sizeof(status));
    rc = isc_dsql_fetch_ptr(status, &stmt_handle, 1, &out_sqlda);
    if (rc != FB_SQL_SUCCESS && rc != FB_SQL_SUCCESS_INFO) {
        /* EOF / zero rows or fetch error => health fail */
        firebird_status_to_error(status, desig);
        goto health_done;
    }

    ok = true;

health_done:
    if (stmt_handle && isc_dsql_free_statement_ptr) {
        memset(status, 0, sizeof(status));
        (void)isc_dsql_free_statement_ptr(status, &stmt_handle, FB_DSQL_DEALLOCATE);
        stmt_handle = NULL;
    }

    if (own_health_txn && fb_conn->tr_handle) {
        memset(status, 0, sizeof(status));
        if (ok && isc_commit_transaction_ptr) {
            fb_status_t commit_result = isc_commit_transaction_ptr(status, &fb_conn->tr_handle);
            fb_conn->tr_handle = NULL;
            if (commit_result != FB_SQL_SUCCESS && commit_result != FB_SQL_SUCCESS_INFO) {
                firebird_status_to_error(status, desig);
                ok = false;
            }
        } else if (isc_rollback_transaction_ptr) {
            (void)isc_rollback_transaction_ptr(status, &fb_conn->tr_handle);
            fb_conn->tr_handle = NULL;
        }
    }

    if (ok) {
        connection->last_health_check = time(NULL);
        connection->consecutive_failures = 0;
        return true;
    }

    connection->consecutive_failures++;
    log_this(desig, "Firebird health check failed", LOG_LEVEL_ERROR, 0);
    return false;
}


bool firebird_reset_connection(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_FIREBIRD) {
        return false;
    }

    connection->status = DB_CONNECTION_CONNECTED;
    connection->connected_since = time(NULL);
    connection->consecutive_failures = 0;

    log_this(firebird_designator_safe(connection), "Firebird connection reset", LOG_LEVEL_TRACE, 0);
    return true;
}

/*
 * ----------------------------------------------------------------------------
 * Watchdog cancel support
 * ----------------------------------------------------------------------------
 * Phase 5 skeleton: no query execution path exists yet, so stmt_handle is
 * always NULL here. The cancel hook logs and returns. Phase 5 sets up the
 * tracking infrastructure (set / clear / cancel) so later phases can plug
 * in real fb_cancel_operation calls once isc_dsql_execute exists.
 */
void firebird_cancel_inflight(DatabaseHandle* connection) {
    if (!connection || connection->engine_type != DB_ENGINE_FIREBIRD) {
        return;
    }

    FirebirdConnection* fb_conn = (FirebirdConnection*)connection->connection_handle;
    if (!fb_conn) {
        return;
    }

    pthread_mutex_lock(&fb_conn->stmt_lock);
    void* stmt = fb_conn->stmt_handle;
    pthread_mutex_unlock(&fb_conn->stmt_lock);

    if (!stmt) {
        return;
    }

    if (fb_cancel_operation_ptr) {
        fb_status_t status[FB_STATUS_LENGTH];
        memset(status, 0, sizeof(status));
        fb_status_t result = fb_cancel_operation_ptr(status, &fb_conn->db_handle, FB_CANCEL_CURRENT);
        const char* desig = firebird_designator_safe(connection);
        if (result != FB_SQL_SUCCESS && result != FB_SQL_SUCCESS_INFO) {
            log_this(desig, "Firebird: fb_cancel_operation returned %d", LOG_LEVEL_ERROR, 1, (int)result);
        } else {
            log_this(desig, "Firebird: requested cancel of in-flight query", LOG_LEVEL_ALERT, 0);
        }
    } else {
        log_this(firebird_designator_safe(connection),
                 "Firebird: cancel requested but fb_cancel_operation unavailable (no in-flight statement)",
                 LOG_LEVEL_ALERT, 0);
    }
}

void firebird_active_stmt_set(DatabaseHandle* connection, void* stmt_handle) {
    if (!connection || connection->engine_type != DB_ENGINE_FIREBIRD || !stmt_handle) {
        return;
    }
    FirebirdConnection* fb_conn = (FirebirdConnection*)connection->connection_handle;
    if (!fb_conn) {
        return;
    }
    pthread_mutex_lock(&fb_conn->stmt_lock);
    fb_conn->stmt_handle = stmt_handle;
    pthread_mutex_unlock(&fb_conn->stmt_lock);
}

void firebird_active_stmt_clear(DatabaseHandle* connection, const void* stmt_handle) {
    if (!connection || connection->engine_type != DB_ENGINE_FIREBIRD) {
        return;
    }
    FirebirdConnection* fb_conn = (FirebirdConnection*)connection->connection_handle;
    if (!fb_conn) {
        return;
    }
    pthread_mutex_lock(&fb_conn->stmt_lock);
    if (fb_conn->stmt_handle == stmt_handle) {
        fb_conn->stmt_handle = NULL;
    }
    pthread_mutex_unlock(&fb_conn->stmt_lock);
}

/*
 * ----------------------------------------------------------------------------
 * Local helpers
 * ----------------------------------------------------------------------------
 */
const char* firebird_designator_safe(const DatabaseHandle* connection) {
    return connection && connection->designator ? connection->designator : SR_DATABASE;
}
