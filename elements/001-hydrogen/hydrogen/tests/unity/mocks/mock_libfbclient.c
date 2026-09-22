/*
 * Mock libfbclient functions for unit testing
 *
 * This file provides mock implementations of Firebird client (isc_*)
 * functions to enable testing of Firebird database operations without
 * a live Firebird installation.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <stddef.h>

#include "mock_libfbclient.h"

/*
 * Mock control state — each function returns its configured result code
 * and optionally sets output handles.
 */
static int mock_isc_attach_database_result = 0;   // 0 = success
static int mock_isc_detach_database_result = 0;
static int mock_isc_start_transaction_result = 0;
static int mock_isc_commit_transaction_result = 0;
static int mock_isc_rollback_transaction_result = 0;
static int mock_isc_dsql_allocate_result = 0;
static int mock_isc_dsql_prepare_result = 0;
static int mock_isc_dsql_execute_result = 0;
static int mock_isc_dsql_execute_immediate_result = 0;
static int mock_isc_dsql_free_statement_result = 0;
static int mock_isc_dsql_fetch_result = 0;
static int mock_isc_dsql_describe_bind_result = 0;
static int mock_fb_cancel_operation_result = 0;

// Output handle for attach (so callers see a "real" handle)
static void* mock_isc_attach_database_output_handle = (void*)0xDEADBEEF;

// Call counters
static int mock_isc_attach_database_calls = 0;
static int mock_isc_detach_database_calls = 0;
static int mock_isc_start_transaction_calls = 0;
static int mock_isc_commit_transaction_calls = 0;
static int mock_isc_rollback_transaction_calls = 0;
static int mock_isc_dsql_allocate_calls = 0;
static int mock_isc_dsql_prepare_calls = 0;
static int mock_isc_dsql_execute_calls = 0;
static int mock_isc_dsql_execute_immediate_calls = 0;
static int mock_isc_dsql_free_statement_calls = 0;
static int mock_isc_dsql_fetch_calls = 0;
static int mock_isc_dsql_describe_bind_calls = 0;
static int mock_fb_cancel_operation_calls = 0;

// Last attach args (for assertion in tests)
static const char* last_attach_dbname = NULL;
static const char* last_attach_params = NULL;

/*
 * ----------------------------------------------------------------------------
 * Mock isc_* functions
 * ----------------------------------------------------------------------------
 * Signatures match the typedefs in firebird/types.h exactly so they can
 * be assigned to the _ptr function-pointer variables without warnings.
 */
fb_status_t mock_isc_attach_database(fb_status_t* status, short name_length, const char* name,
                                      void** db_handle, short param_length, const char* params) {
    mock_isc_attach_database_calls++;
    last_attach_dbname = name;
    last_attach_params = params;
    (void)name_length;
    (void)param_length;

    if (status) {
        memset(status, 0, 20 * sizeof(fb_status_t));
    }
    if (db_handle) {
        *db_handle = mock_isc_attach_database_output_handle;
    }
    return (fb_status_t)mock_isc_attach_database_result;
}

fb_status_t mock_isc_detach_database(fb_status_t* status, void** db_handle) {
    mock_isc_detach_database_calls++;
    (void)db_handle;
    if (status) {
        memset(status, 0, 20 * sizeof(fb_status_t));
    }
    return (fb_status_t)mock_isc_detach_database_result;
}

fb_status_t mock_isc_start_transaction(fb_status_t* status, void** tr_handle, short num_db, ...) {
    mock_isc_start_transaction_calls++;
    (void)num_db;
    if (status) {
        memset(status, 0, 20 * sizeof(fb_status_t));
    }
    if (tr_handle && mock_isc_start_transaction_result == 0) {
        *tr_handle = (void*)0xBEEF0001;
    }
    return (fb_status_t)mock_isc_start_transaction_result;
}

fb_status_t mock_isc_commit_transaction(fb_status_t* status, void** tr_handle) {
    mock_isc_commit_transaction_calls++;
    if (status) {
        memset(status, 0, 20 * sizeof(fb_status_t));
    }
    if (tr_handle && mock_isc_commit_transaction_result == 0) {
        *tr_handle = NULL;
    }
    return (fb_status_t)mock_isc_commit_transaction_result;
}

fb_status_t mock_isc_rollback_transaction(fb_status_t* status, void** tr_handle) {
    mock_isc_rollback_transaction_calls++;
    if (status) {
        memset(status, 0, 20 * sizeof(fb_status_t));
    }
    if (tr_handle && mock_isc_rollback_transaction_result == 0) {
        *tr_handle = NULL;
    }
    return (fb_status_t)mock_isc_rollback_transaction_result;
}

fb_status_t mock_isc_dsql_allocate(fb_status_t* status, void** db_handle, void** stmt) {
    mock_isc_dsql_allocate_calls++;
    (void)db_handle;
    if (status) {
        memset(status, 0, 20 * sizeof(fb_status_t));
    }
    if (stmt && mock_isc_dsql_allocate_result == 0) {
        *stmt = (void*)0xBEEF0002;
    }
    return (fb_status_t)mock_isc_dsql_allocate_result;
}

fb_status_t mock_isc_dsql_prepare(fb_status_t* status, void** tr_handle, void** stmt_handle,
                                   short length, const char* sql, short dialect, void* xsqlda) {
    mock_isc_dsql_prepare_calls++;
    (void)tr_handle;
    (void)stmt_handle;
    (void)length;
    (void)sql;
    (void)dialect;
    (void)xsqlda;
    if (status) {
        memset(status, 0, 20 * sizeof(fb_status_t));
    }
    return (fb_status_t)mock_isc_dsql_prepare_result;
}

fb_status_t mock_isc_dsql_execute(fb_status_t* status, void** tr_handle, void** stmt_handle,
                                   short unused1, const void* unused2) {
    mock_isc_dsql_execute_calls++;
    (void)tr_handle;
    (void)stmt_handle;
    (void)unused1;
    (void)unused2;
    if (status) {
        memset(status, 0, 20 * sizeof(fb_status_t));
    }
    return (fb_status_t)mock_isc_dsql_execute_result;
}

fb_status_t mock_isc_dsql_execute_immediate(fb_status_t* status, void** db_handle,
                                             void** tr_handle, short dialect,
                                             const char* sql, short param_count,
                                             const void* xsqlda) {
    mock_isc_dsql_execute_immediate_calls++;
    (void)db_handle;
    (void)tr_handle;
    (void)dialect;
    (void)sql;
    (void)param_count;
    (void)xsqlda;
    if (status) {
        memset(status, 0, 20 * sizeof(fb_status_t));
    }
    return (fb_status_t)mock_isc_dsql_execute_immediate_result;
}

fb_status_t mock_isc_dsql_free_statement(fb_status_t* status, void** stmt_handle, short option) {
    mock_isc_dsql_free_statement_calls++;
    (void)stmt_handle;
    (void)option;
    if (status) {
        memset(status, 0, 20 * sizeof(fb_status_t));
    }
    return (fb_status_t)mock_isc_dsql_free_statement_result;
}

fb_status_t mock_isc_dsql_fetch(fb_status_t* status, void** stmt_handle, short unused1, void* unused2) {
    mock_isc_dsql_fetch_calls++;
    (void)stmt_handle;
    (void)unused1;
    (void)unused2;
    if (status) {
        memset(status, 0, 20 * sizeof(fb_status_t));
    }
    return (fb_status_t)mock_isc_dsql_fetch_result;
}

fb_status_t mock_isc_dsql_describe_bind(fb_status_t* status, void** stmt_handle,
                                        unsigned short da_version, void* xsqlda) {
    mock_isc_dsql_describe_bind_calls++;
    (void)stmt_handle;
    (void)da_version;
    (void)xsqlda;
    if (status) {
        memset(status, 0, 20 * sizeof(fb_status_t));
    }
    return (fb_status_t)mock_isc_dsql_describe_bind_result;
}

void mock_isc_decode_sql_date(const void* nday, void* times_arg) {
    (void)nday;
    if (times_arg) {
        memset(times_arg, 0, sizeof(struct tm));
    }
}

void mock_isc_decode_sql_time(const void* ntime, void* times_arg) {
    (void)ntime;
    if (times_arg) {
        memset(times_arg, 0, sizeof(struct tm));
    }
}

void mock_isc_decode_timestamp(const void* ts, void* times_arg) {
    (void)ts;
    if (times_arg) {
        memset(times_arg, 0, sizeof(struct tm));
    }
}


fb_status_t mock_isc_open_blob2(fb_status_t* status, void** db_handle, void** tr_handle,
                                void** blob_handle, void* blob_id, short bpb_len, const char* bpb) {
    (void)db_handle; (void)tr_handle; (void)blob_id; (void)bpb_len; (void)bpb;
    if (status) { status[0] = 0; status[1] = 0; }
    if (blob_handle) *blob_handle = (void*)0xB10BU;
    return 0;
}

fb_status_t mock_isc_get_segment(fb_status_t* status, void** blob_handle,
                                 unsigned short* actual_len, unsigned short buf_len, char* buf) {
    (void)blob_handle; (void)buf_len; (void)buf;
    if (actual_len) *actual_len = 0;
    if (status) { status[0] = 1; status[1] = 335544367; } /* isc_segstr_eof */
    return 335544367;
}

fb_status_t mock_isc_close_blob(fb_status_t* status, void** blob_handle) {
    if (blob_handle) *blob_handle = NULL;
    if (status) { status[0] = 0; status[1] = 0; }
    return 0;
}

long mock_fb_interpret(char* buf, unsigned int buflen, const fb_status_t** status_vector) {
    (void)status_vector;
    if (buf && buflen > 0) {
        buf[0] = '\0';
    }
    return 0; /* no message lines in default mock */
}

fb_status_t mock_fb_cancel_operation(fb_status_t* status, void** db_handle, unsigned short option) {
    mock_fb_cancel_operation_calls++;
    (void)db_handle;
    (void)option;
    if (status) {
        memset(status, 0, 20 * sizeof(fb_status_t));
    }
    return (fb_status_t)mock_fb_cancel_operation_result;
}

/*
 * ----------------------------------------------------------------------------
 * Mock control functions
 * ----------------------------------------------------------------------------
 */
void mock_libfbc_reset_all(void) {
    mock_isc_attach_database_result = 0;
    mock_isc_detach_database_result = 0;
    mock_isc_start_transaction_result = 0;
    mock_isc_commit_transaction_result = 0;
    mock_isc_rollback_transaction_result = 0;
    mock_isc_dsql_allocate_result = 0;
    mock_isc_dsql_prepare_result = 0;
    mock_isc_dsql_execute_result = 0;
    mock_isc_dsql_execute_immediate_result = 0;
    mock_isc_dsql_free_statement_result = 0;
    mock_isc_dsql_fetch_result = 0;
    mock_isc_dsql_describe_bind_result = 0;
    mock_fb_cancel_operation_result = 0;

    mock_isc_attach_database_output_handle = (void*)0xDEADBEEF;

    mock_isc_attach_database_calls = 0;
    mock_isc_detach_database_calls = 0;
    mock_isc_start_transaction_calls = 0;
    mock_isc_commit_transaction_calls = 0;
    mock_isc_rollback_transaction_calls = 0;
    mock_isc_dsql_allocate_calls = 0;
    mock_isc_dsql_prepare_calls = 0;
    mock_isc_dsql_execute_calls = 0;
    mock_isc_dsql_execute_immediate_calls = 0;
    mock_isc_dsql_free_statement_calls = 0;
    mock_isc_dsql_fetch_calls = 0;
    mock_isc_dsql_describe_bind_calls = 0;
    mock_fb_cancel_operation_calls = 0;

    last_attach_dbname = NULL;
    last_attach_params = NULL;
}

void mock_libfbc_set_isc_attach_database_result(int result) {
    mock_isc_attach_database_result = result;
}

void mock_libfbc_set_isc_detach_database_result(int result) {
    mock_isc_detach_database_result = result;
}

void mock_libfbc_set_isc_start_transaction_result(int result) {
    mock_isc_start_transaction_result = result;
}

void mock_libfbc_set_isc_commit_transaction_result(int result) {
    mock_isc_commit_transaction_result = result;
}

void mock_libfbc_set_isc_rollback_transaction_result(int result) {
    mock_isc_rollback_transaction_result = result;
}

void mock_libfbc_set_isc_dsql_allocate_result(int result) {
    mock_isc_dsql_allocate_result = result;
}

void mock_libfbc_set_isc_dsql_prepare_result(int result) {
    mock_isc_dsql_prepare_result = result;
}

void mock_libfbc_set_isc_dsql_execute_result(int result) {
    mock_isc_dsql_execute_result = result;
}

void mock_libfbc_set_isc_dsql_execute_immediate_result(int result) {
    mock_isc_dsql_execute_immediate_result = result;
}

void mock_libfbc_set_isc_dsql_free_statement_result(int result) {
    mock_isc_dsql_free_statement_result = result;
}

void mock_libfbc_set_isc_dsql_fetch_result(int result) {
    mock_isc_dsql_fetch_result = result;
}

void mock_libfbc_set_isc_dsql_describe_bind_result(int result) {
    mock_isc_dsql_describe_bind_result = result;
}

void mock_libfbc_set_fb_cancel_operation_result(int result) {
    mock_fb_cancel_operation_result = result;
}

int mock_libfbc_get_isc_attach_database_call_count(void) {
    return mock_isc_attach_database_calls;
}

int mock_libfbc_get_isc_detach_database_call_count(void) {
    return mock_isc_detach_database_calls;
}

int mock_libfbc_get_isc_dsql_execute_immediate_call_count(void) {
    return mock_isc_dsql_execute_immediate_calls;
}

void mock_libfbc_get_last_attach_args(const char** dbname, const char** params) {
    if (dbname)  *dbname  = last_attach_dbname;
    if (params)  *params  = last_attach_params;
}
