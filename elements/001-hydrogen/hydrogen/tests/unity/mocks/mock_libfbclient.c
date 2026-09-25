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
#include <src/database/firebird/query_internal.h>

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
static fb_xsqlda_min* mock_describe_bind_sqlda_ptr = NULL;
static short mock_describe_bind_set_sqld = 0;
static int mock_describe_bind_should_set_sqld = 0;
static short mock_describe_bind_sqltype = FB_SQL_TEXT;
static int mock_fb_cancel_operation_result = 0;
/* isc_dsql_sql_info mock state */
static int mock_isc_dsql_sql_info_result = 0;
static unsigned char mock_sql_info_data[32] = {0};
static short mock_sql_info_data_len = 0;
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
static int mock_isc_dsql_execute2_calls = 0;
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
     free((void*)last_attach_dbname);
     last_attach_dbname = name ? strdup(name) : NULL;
     free((void*)last_attach_params);
     last_attach_params = params ? strdup(params) : NULL;
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

int mock_isc_dsql_sql_info_calls = 0;

fb_status_t mock_isc_dsql_sql_info(fb_status_t* status, void** stmt_handle,
                                    short item_len, const char* items,
                                    short buf_len, char* buf) {
    mock_isc_dsql_sql_info_calls++;
    (void)stmt_handle;
    (void)item_len;
    (void)items;
    if (status) {
        memset(status, 0, 20 * sizeof(fb_status_t));
    }
    /* Fill the buffer with mock data if configured */
    if (buf && mock_sql_info_data_len > 0 && mock_sql_info_data_len <= buf_len) {
        memcpy(buf, mock_sql_info_data, (size_t)mock_sql_info_data_len);
    } else if (buf && buf_len > 0) {
        memset(buf, 0, (size_t)buf_len);
    }
    return (fb_status_t)mock_isc_dsql_sql_info_result;
}

fb_status_t mock_isc_dsql_execute2(fb_status_t* status, void** tr_handle, void** stmt_handle,
                                     short unused1, const void* unused2, const void* unused3) {
    mock_isc_dsql_execute2_calls++;
    (void)tr_handle;
    (void)stmt_handle;
    (void)unused1;
    (void)unused2;
    (void)unused3;
    if (status) {
        memset(status, 0, 20 * sizeof(fb_status_t));
    }
    return (fb_status_t)mock_isc_dsql_execute_result;
}

fb_status_t mock_isc_dsql_describe_bind(fb_status_t* status, void** stmt_handle,
                                        unsigned short da_version, void* xsqlda) {
    mock_isc_dsql_describe_bind_calls++;
    (void)stmt_handle;
    (void)da_version;

    if (mock_describe_bind_should_set_sqld && xsqlda) {
        fb_xsqlda_min* sqlda = (fb_xsqlda_min*)xsqlda;
        /* Only set sqld — sqln stays at the initial allocation value.
           This simulates Firebird returning sqld > sqln, which triggers
           the reallocation path in firebird_build_input_sqlda. */
        sqlda->sqld = mock_describe_bind_set_sqld;
        /* Initialize sqlvar entries up to sqln (not sqld) */
        for (short ci = 0; ci < sqlda->sqln && ci < sqlda->sqld; ci++) {
            sqlda->sqlvar[ci].sqltype = mock_describe_bind_sqltype;
            sqlda->sqlvar[ci].sqllen = 4;
        }
        mock_describe_bind_sqlda_ptr = sqlda;
    }

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

void mock_isc_encode_sql_date(const void* tm_in, void* out) {
    (void)tm_in;
    if (out) {
        memset(out, 0xAB, 4);
    }
}

void mock_isc_encode_sql_time(const void* tm_in, void* out) {
    (void)tm_in;
    if (out) {
        memset(out, 0xCD, 4);
    }
}

void mock_isc_encode_timestamp(const void* tm_in, void* out) {
    (void)tm_in;
    if (out) {
        memset(out, 0xEF, 8);
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
    mock_describe_bind_sqlda_ptr = NULL;
    mock_describe_bind_should_set_sqld = 0;
    mock_describe_bind_set_sqld = 0;
    mock_describe_bind_sqltype = FB_SQL_TEXT;
    mock_fb_cancel_operation_result = 0;
    mock_isc_dsql_sql_info_result = 0;
    mock_sql_info_data_len = 0;
    mock_isc_dsql_sql_info_calls = 0;
    memset(mock_sql_info_data, 0, sizeof(mock_sql_info_data));
    mock_isc_dsql_execute_calls = 0;
    mock_isc_dsql_execute2_calls = 0;

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

     free((void*)last_attach_dbname);
     free((void*)last_attach_params);
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

void mock_libfbc_set_isc_dsql_sql_info_result(int result) {
    mock_isc_dsql_sql_info_result = result;
}

void mock_libfbc_set_isc_dsql_sql_info_data(const unsigned char* data, short len) {
    if (len > 0 && len <= (short)sizeof(mock_sql_info_data) && data) {
        memcpy(mock_sql_info_data, data, (size_t)len);
        mock_sql_info_data_len = len;
    } else {
        mock_sql_info_data_len = 0;
    }
}

void mock_libfbc_set_isc_dsql_describe_bind_sqlda(void* sqlda) {
    mock_describe_bind_sqlda_ptr = (fb_xsqlda_min*)sqlda;
}

void mock_libfbc_set_isc_dsql_describe_bind_set_sqld(short sqld) {
    mock_describe_bind_should_set_sqld = 1;
    mock_describe_bind_set_sqld = sqld;
}

void mock_libfbc_set_isc_dsql_describe_bind_sqltype(short sqltype) {
    mock_describe_bind_sqltype = sqltype;
}

void* mock_libfbc_get_isc_dsql_describe_bind_sqlda(void) {
    return mock_describe_bind_sqlda_ptr;
}

void mock_libfbc_set_fb_cancel_operation_result(int result) {
    mock_fb_cancel_operation_result = result;
}

void mock_libfbc_set_isc_dsql_execute2_result(int result) {
    mock_isc_dsql_execute_result = result;
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

int mock_libfbc_get_isc_dsql_execute_call_count(void) {
    return mock_isc_dsql_execute_calls;
}

int mock_libfbc_get_isc_dsql_execute2_call_count(void) {
    return mock_isc_dsql_execute2_calls;
}

int mock_libfbc_get_fb_cancel_operation_call_count(void) {
    return mock_fb_cancel_operation_calls;
}

void mock_libfbc_get_last_attach_args(const char** dbname, const char** params) {
    if (dbname)  *dbname  = last_attach_dbname;
    if (params)  *params  = last_attach_params;
}
