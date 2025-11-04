// Database Engine Header
// Function declarations for database engine abstraction layer

#ifndef DATABASE_ENGINE_H
#define DATABASE_ENGINE_H

#include <src/hydrogen.h>
#include <src/database/database_types.h>

// Forward declarations to avoid circular dependencies
typedef struct DatabaseHandle DatabaseHandle;
typedef struct ConnectionConfig ConnectionConfig;
typedef struct QueryRequest QueryRequest;
typedef struct QueryResult QueryResult;
typedef struct Transaction Transaction;
typedef struct PreparedStatement PreparedStatement;
typedef struct DatabaseEngineInterface DatabaseEngineInterface;

// Engine management functions
bool database_engine_init(void);
bool database_engine_register(DatabaseEngineInterface* engine);
DatabaseEngineInterface* database_engine_get(DatabaseEngine engine_type);
DatabaseEngineInterface* database_engine_get_with_designator(DatabaseEngine engine_type, const char* designator);
DatabaseEngineInterface* database_engine_get_by_name(const char* name);

// Connection management
bool database_engine_connect(DatabaseEngine engine_type, ConnectionConfig* config, DatabaseHandle** connection);
bool database_engine_connect_with_designator(DatabaseEngine engine_type, ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
bool database_engine_health_check(DatabaseHandle* connection);

// Query execution
bool database_engine_execute(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);

// Transaction management
bool database_engine_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction);
bool database_engine_commit_transaction(DatabaseHandle* connection, Transaction* transaction);
bool database_engine_rollback_transaction(DatabaseHandle* connection, Transaction* transaction);

// Connection string utilities
char* database_engine_build_connection_string(DatabaseEngine engine_type, ConnectionConfig* config);
bool database_engine_validate_connection_string(DatabaseEngine engine_type, const char* connection_string);

// Cleanup functions
void database_engine_cleanup_connection(DatabaseHandle* connection);
void database_engine_cleanup_result(QueryResult* result);
void database_engine_cleanup_transaction(Transaction* transaction);

// Utility functions
void database_get_counts_by_type(int* postgres_count, int* mysql_count, int* sqlite_count, int* db2_count);
void database_get_supported_engines(char* buffer, size_t buffer_size);

#endif // DATABASE_ENGINE_H