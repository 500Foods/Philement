/*
 * Mock Database Engine functions for unit testing
 *
 * This file provides mock implementations of database_engine_* functions
 * to enable testing of database operations without real database connections.
 */

#ifndef MOCK_DATABASE_ENGINE_H
#define MOCK_DATABASE_ENGINE_H

#include <src/database/database.h>

// Enable mock override
#ifdef USE_MOCK_DATABASE_ENGINE
#define database_engine_begin_transaction mock_database_engine_begin_transaction
#define database_engine_commit_transaction mock_database_engine_commit_transaction
#define database_engine_rollback_transaction mock_database_engine_rollback_transaction
#define database_engine_execute mock_database_engine_execute
#define database_engine_cleanup_result mock_database_engine_cleanup_result
#define database_engine_cleanup_transaction mock_database_engine_cleanup_transaction
#define database_engine_get mock_database_engine_get
#define database_engine_health_check mock_database_engine_health_check
#define database_get_readiness mock_database_get_readiness
#endif

// Mock function prototypes - match signatures from database.h
bool mock_database_engine_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool mock_database_engine_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool mock_database_engine_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);
bool mock_database_engine_execute(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
void mock_database_engine_cleanup_result(QueryResult* result);
void mock_database_engine_cleanup_transaction(Transaction* transaction);
DatabaseEngineInterface* mock_database_engine_get(DatabaseEngine engine_type);
bool mock_database_engine_health_check(DatabaseHandle* connection);
bool mock_database_get_readiness(DatabaseReadiness* readiness);
void mock_database_engine_cancel_inflight(DatabaseHandle* connection);
int mock_database_engine_get_cancel_call_count(void);
void mock_database_engine_reset_cancel_call_count(void);

// Mock control functions
void mock_database_engine_reset_all(void);
void mock_database_engine_set_begin_result(bool result);
void mock_database_engine_set_commit_result(bool result);
void mock_database_engine_set_rollback_result(bool result);
void mock_database_engine_set_execute_result(bool success);
void mock_database_engine_set_execute_query_result(QueryResult* result);
void mock_database_engine_set_execute_json_data(const char* json_data);
void mock_database_engine_set_affected_rows(int rows);
void mock_database_engine_set_health_check_result(bool result);
void mock_database_engine_set_execute_error_class(DatabaseErrorClass err_class);
int mock_database_engine_get_execute_call_count(void);
void mock_database_engine_reset_execute_call_count(void);

// database_get_readiness override: when armed, mock_database_get_readiness
// copies the provided snapshot into the caller's buffer and returns all_ready.
void mock_database_set_readiness(const DatabaseReadiness* snapshot, bool all_ready);
void mock_database_get_readiness_reset(void);

#endif // MOCK_DATABASE_ENGINE_H