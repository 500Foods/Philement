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
#endif

// Mock function prototypes - match signatures from database.h
bool mock_database_engine_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool mock_database_engine_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool mock_database_engine_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);
bool mock_database_engine_execute(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
void mock_database_engine_cleanup_result(QueryResult* result);
void mock_database_engine_cleanup_transaction(Transaction* transaction);
DatabaseEngineInterface* mock_database_engine_get(DatabaseEngine engine_type);

// Mock control functions
void mock_database_engine_reset_all(void);
void mock_database_engine_set_begin_result(bool result);
void mock_database_engine_set_commit_result(bool result);
void mock_database_engine_set_rollback_result(bool result);
void mock_database_engine_set_execute_result(bool success);
void mock_database_engine_set_execute_query_result(QueryResult* result);
void mock_database_engine_set_execute_json_data(const char* json_data);
void mock_database_engine_set_affected_rows(int rows);

#endif // MOCK_DATABASE_ENGINE_H