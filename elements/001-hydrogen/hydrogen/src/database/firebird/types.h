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
typedef fb_status_t (*isc_detach_database_t)(fb_status_t*, void*);

/* Transactions */
typedef fb_status_t (*isc_start_transaction_t)(fb_status_t*, void*, void**, int, const char*);
typedef fb_status_t (*isc_commit_transaction_t)(fb_status_t*, void*);
typedef fb_status_t (*isc_rollback_transaction_t)(fb_status_t*, void*);

/* DSQL (dynamic SQL) */
typedef fb_status_t (*isc_dsql_allocate_t)(fb_status_t*, void*, short, void*);
typedef fb_status_t (*isc_dsql_prepare_t)(fb_status_t*, void*, void*, short, const char*, short, const char*);
typedef fb_status_t (*isc_dsql_execute_t)(fb_status_t*, void*, void*, short, const char*, short);
typedef fb_status_t (*isc_dsql_execute_immediate_t)(fb_status_t*, void*, void*, short, const char*, short);
typedef fb_status_t (*isc_dsql_free_statement_t)(fb_status_t*, void*, short);

/* Cursor-style fetch */
typedef fb_status_t (*isc_dsql_fetch_t)(fb_status_t*, void*, short, void*);

/* Cancel */
typedef fb_status_t (*fb_cancel_operation_t)(fb_status_t*, void*, unsigned int);

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
extern isc_dsql_execute_immediate_t   isc_dsql_execute_immediate_ptr;
extern isc_dsql_free_statement_t      isc_dsql_free_statement_ptr;
extern isc_dsql_fetch_t               isc_dsql_fetch_ptr;
extern fb_cancel_operation_t          fb_cancel_operation_ptr;

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

/* fb_cancel_operation flag */
#define FB_CANCEL_CURRENT       1

/* Firebird DSQL info constants (kept minimal) */
#define FB_DSQL_CLOSE           1
#define FB_DSQL_DEALLOCATE      2
#define FB_DSQL_DROP_INV         8

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
