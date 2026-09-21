/*
 * Mock libfbclient functions for unit testing
 *
 * This file provides mock implementations of Firebird client (isc_*)
 * functions to enable testing of Firebird database operations without
 * a live Firebird installation.
 *
 * Mock function signatures must match the typedef signatures in
 * firebird/types.h exactly, so they can be assigned to the _ptr
 * function-pointer variables without -Wcast-function-type warnings.
 */

#ifndef MOCK_LIBFBC_H
#define MOCK_LIBFBC_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * fb_status_t is typedef'd as `intptr_t` in firebird/types.h.
 * We redefine it here so the mock header is self-contained.
 */
typedef intptr_t fb_status_t;

/*
 * Mock function prototypes — must match types.h typedefs exactly.
 * The _ptr variables in connection.c are assigned to point at these
 * during mock builds via load_libfbclient_functions().
 */

/* Attachment / detach */
fb_status_t mock_isc_attach_database(fb_status_t*, short, const char*, void**, short, const char*);
fb_status_t mock_isc_detach_database(fb_status_t*, void*);

/* Transactions */
fb_status_t mock_isc_start_transaction(fb_status_t*, void*, void**, int, const char*);
fb_status_t mock_isc_commit_transaction(fb_status_t*, void*);
fb_status_t mock_isc_rollback_transaction(fb_status_t*, void*);

/* DSQL */
fb_status_t mock_isc_dsql_allocate(fb_status_t*, void*, short, void*);
fb_status_t mock_isc_dsql_prepare(fb_status_t*, void*, void*, short, const char*, short, const char*);
fb_status_t mock_isc_dsql_execute(fb_status_t*, void*, void*, short, const char*, short);
fb_status_t mock_isc_dsql_execute_immediate(fb_status_t*, void*, void*, short, const char*, short);
fb_status_t mock_isc_dsql_free_statement(fb_status_t*, void*, short);
fb_status_t mock_isc_dsql_fetch(fb_status_t*, void*, short, void*);

/* Cancel */
fb_status_t mock_fb_cancel_operation(fb_status_t*, void*, unsigned int);

/*
 * Mock control functions for tests
 */
void mock_libfbc_reset_all(void);

void mock_libfbc_set_isc_attach_database_result(int result);
void mock_libfbc_set_isc_detach_database_result(int result);
void mock_libfbc_set_isc_start_transaction_result(int result);
void mock_libfbc_set_isc_commit_transaction_result(int result);
void mock_libfbc_set_isc_rollback_transaction_result(int result);
void mock_libfbc_set_isc_dsql_allocate_result(int result);
void mock_libfbc_set_isc_dsql_prepare_result(int result);
void mock_libfbc_set_isc_dsql_execute_result(int result);
void mock_libfbc_set_isc_dsql_execute_immediate_result(int result);
void mock_libfbc_set_isc_dsql_free_statement_result(int result);
void mock_libfbc_set_isc_dsql_fetch_result(int result);
void mock_libfbc_set_fb_cancel_operation_result(int result);

int  mock_libfbc_get_isc_attach_database_call_count(void);
int  mock_libfbc_get_isc_detach_database_call_count(void);
int  mock_libfbc_get_isc_dsql_execute_immediate_call_count(void);
void mock_libfbc_get_last_attach_args(const char** dbname, const char** params);

#endif // MOCK_LIBFBC_H
