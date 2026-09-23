/*
 * Firebird Database Engine - Type Definitions Header
 *
 * Header file for Firebird engine type definitions and isc_* function pointer types.
 * Uses libfbclient (ibase.h / isc_* API). Function pointers are loaded via
 * dlopen at runtime, or mocked in Unity tests via USE_MOCK_LIBFBC.
 */

#ifndef DATABASE_ENGINE_FIREBIRD_TYPES_H
#define DATABASE_ENGINE_FIREBIRD_TYPES_H

#include <src/database/database.h>
#include <stdint.h>

/*
 * Firebird / IBase type aliases (we do not include <ibase.h> in the build
 * to avoid requiring firebird-devel on every dev box; Unity tests mock
 * these via USE_MOCK_LIBFBC). The real libfbclient signatures use:
 *   ISC_STATUS = intptr_t (typically long on 64-bit).
 * We use intptr_t so the status vector has the correct element size.
 */
typedef intptr_t fb_status_t;       /* ISC_STATUS (array element) */
typedef unsigned char fb_uchar_t;

/*
 * isc_* function pointer typedefs.
 * These mirror the libfbclient / ibase.h signatures closely enough
 * for our usage (connect, execute, fetches, blobs).
 */

/* Attachment / detach */
typedef fb_status_t (*isc_attach_database_t)(fb_status_t*, short, const char*, void**, short, const char*);
typedef fb_status_t (*isc_detach_database_t)(fb_status_t*, void**);

/* Transactions */
typedef fb_status_t (*isc_start_transaction_t)(fb_status_t*, void**, short, ...);
typedef fb_status_t (*isc_commit_transaction_t)(fb_status_t*, void**);
typedef fb_status_t (*isc_rollback_transaction_t)(fb_status_t*, void**);

/* DSQL (dynamic SQL) */
typedef fb_status_t (*isc_dsql_allocate_t)(fb_status_t*, void**, void**);  /* isc_dsql_allocate_statement */
typedef fb_status_t (*isc_dsql_prepare_t)(fb_status_t*, void**, void**, short, const char*, short, void*);
typedef fb_status_t (*isc_dsql_execute_t)(fb_status_t*, void**, void**, short, const void*);
/* execute2 takes input SQLDA then output SQLDA. Required for singleton RETURNING. */
typedef fb_status_t (*isc_dsql_execute2_t)(fb_status_t*, void**, void**, short, const void*, const void*);
typedef fb_status_t (*isc_dsql_execute_immediate_t)(fb_status_t*, void**, void**, short, const char*, short, const void*);
typedef fb_status_t (*isc_dsql_free_statement_t)(fb_status_t*, void**, short);
typedef fb_status_t (*isc_dsql_fetch_t)(fb_status_t*, void**, short, void*);
/* isc_dsql_sql_info(status, &stmt, item_len, items, buf_len, buf) */
typedef fb_status_t (*isc_dsql_sql_info_t)(fb_status_t*, void**, short, const char*, short, char*);
typedef fb_status_t (*isc_dsql_describe_bind_t)(fb_status_t*, void**, unsigned short, void*);
/* Decode ISC_DATE / ISC_TIME / ISC_TIMESTAMP into struct tm (void return). */
typedef void (*isc_decode_sql_date_t)(const void*, void*);
typedef void (*isc_decode_sql_time_t)(const void*, void*);
typedef void (*isc_decode_timestamp_t)(const void*, void*);
/* Encode struct tm into ISC_DATE / ISC_TIME / ISC_TIMESTAMP. */
typedef void (*isc_encode_sql_date_t)(const void*, void*);
typedef void (*isc_encode_sql_time_t)(const void*, void*);
typedef void (*isc_encode_timestamp_t)(const void*, void*);

/* Cancel */
typedef fb_status_t (*fb_cancel_operation_t)(fb_status_t*, void**, unsigned short);

/* Blobs */
typedef fb_status_t (*isc_open_blob2_t)(fb_status_t*, void**, void**, void**, void*, short, const char*);
typedef fb_status_t (*isc_get_segment_t)(fb_status_t*, void**, unsigned short*, unsigned short, char*);
typedef fb_status_t (*isc_close_blob_t)(fb_status_t*, void**);
/* fb_interpret: decode status vector messages (buf, buflen, &status_ptr) */
typedef long (*fb_interpret_t)(char*, unsigned int, const fb_status_t**);

/* ISC_QUAD stand-in (8 bytes) */
typedef struct {
    int32_t high;
    int32_t low;
} fb_quad_t;

/*
 * Firebird function pointers — loaded via dlopen (real) or assigned from
 * mocks (USE_MOCK_LIBFBC). The variables are defined in connection.c.
 * In mock builds, connection.c assigns them to mock_* functions.
 */
extern isc_attach_database_t          isc_attach_database_ptr;
extern isc_detach_database_t          isc_detach_database_ptr;
extern isc_start_transaction_t        isc_start_transaction_ptr;
extern isc_commit_transaction_t       isc_commit_transaction_ptr;
extern isc_rollback_transaction_t     isc_rollback_transaction_ptr;
extern isc_dsql_allocate_t            isc_dsql_allocate_ptr;
extern isc_dsql_prepare_t             isc_dsql_prepare_ptr;
extern isc_dsql_execute_t             isc_dsql_execute_ptr;
extern isc_dsql_execute2_t            isc_dsql_execute2_ptr;
extern isc_dsql_execute_immediate_t   isc_dsql_execute_immediate_ptr;
extern isc_dsql_free_statement_t      isc_dsql_free_statement_ptr;
extern isc_dsql_fetch_t               isc_dsql_fetch_ptr;
extern isc_dsql_sql_info_t            isc_dsql_sql_info_ptr;
extern isc_dsql_describe_bind_t        isc_dsql_describe_bind_ptr;
extern isc_decode_sql_date_t           isc_decode_sql_date_ptr;
extern isc_decode_sql_time_t           isc_decode_sql_time_ptr;
extern isc_decode_timestamp_t          isc_decode_timestamp_ptr;
extern isc_encode_sql_date_t           isc_encode_sql_date_ptr;
extern isc_encode_sql_time_t           isc_encode_sql_time_ptr;
extern isc_encode_timestamp_t          isc_encode_timestamp_ptr;
extern fb_cancel_operation_t          fb_cancel_operation_ptr;
extern isc_open_blob2_t               isc_open_blob2_ptr;
extern isc_get_segment_t              isc_get_segment_ptr;
extern isc_close_blob_t               isc_close_blob_ptr;
extern fb_interpret_t                 fb_interpret_ptr;

/*
 * Firebird-specific connection wrapper.
 *  - isc_status[20]  : status vector for isc_* calls
 *  - db_handle       : isc_db_handle (opaque attachment handle)
 *  - tr_handle       : isc_tr_handle (opaque transaction handle, or NULL when autocommit)
 *  - stmt_handle     : current in-flight DSQL statement handle (for watchdog cancel)
 *  - stmt_lock       : guards stmt_handle read/write
 */
typedef struct FirebirdConnection {
    fb_status_t isc_status[20];
    void* db_handle;
    void* tr_handle;
    void* stmt_handle;
    pthread_mutex_t stmt_lock;
} FirebirdConnection;

/* DSQL result codes we care about */
#define FB_SQL_SUCCESS          0
#define FB_SQL_SUCCESS_INFO     1

/* isc_open_blob / isc_fetch_blob sub-constants */
#define FB_BLOB_OK               0
#define FB_BLOB_EOF              1
#define FB_ISC_SEGMENT         335544366
#define FB_ISC_SEGSTR_EOF      335544367

/* fb_cancel_operation flag */
#define FB_CANCEL_CURRENT       1

/* Firebird DSQL info constants (kept minimal) */
#define FB_DSQL_CLOSE           1
#define FB_DSQL_DEALLOCATE      2
#define FB_DSQL_DROP_INV         8

/* isc_dsql_sql_info item and statement-type values (inf_pub.h). */
#define FB_INFO_SQL_STMT_TYPE    21
#define FB_STMT_SELECT            1
#define FB_STMT_EXEC_PROCEDURE    8
#define FB_STMT_SELECT_FOR_UPD   12
/* isc_info_sql_records cluster: insert/update/delete counts after DML. */
#define FB_INFO_SQL_RECORDS      23
#define FB_INFO_END               1
#define FB_INFO_REQ_INSERT_COUNT 14
#define FB_INFO_REQ_UPDATE_COUNT 15
#define FB_INFO_REQ_DELETE_COUNT 16

/* Firebird isc_status sentinel: 20-element status vector */
#define FB_STATUS_LENGTH        20

/*
 * Firebird DPB (Database Parameter Buffer) constants.
 * These mirror the values in ibase.h / consts_pub.h so we do
 * not need to include ibase.h in the build (kept from design).
 */
#define FB_DPB_VERSION1         1
#define FB_DPB_USER_NAME        28
#define FB_DPB_PASSWORD         29
#define FB_DPB_SQL_ROLE_NAME    76

#endif // DATABASE_ENGINE_FIREBIRD_TYPES_H
