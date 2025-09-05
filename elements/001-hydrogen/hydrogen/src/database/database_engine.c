/*
 * Database Engine Abstraction Layer
 *
 * Implements the multi-engine interface layer for database operations.
 * Provides unified interface for PostgreSQL, SQLite, MySQL, DB2, and future engines.
 */

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "../hydrogen.h"
#include "database.h"
#include "database_queue.h"

// Global engine registry
static DatabaseEngineInterface* engine_registry[DB_ENGINE_MAX] = {NULL};
static pthread_mutex_t engine_registry_lock = PTHREAD_MUTEX_INITIALIZER;
static bool engine_system_initialized = false;

/*
 * Engine Registry Management
 */

bool database_engine_init(void) {
    if (engine_system_initialized) {
        return true;
    }

    pthread_mutex_lock(&engine_registry_lock);

    // Initialize registry to NULL
    memset(engine_registry, 0, sizeof(engine_registry));

    engine_system_initialized = true;

    pthread_mutex_unlock(&engine_registry_lock);

    log_this(SR_DATABASE, "Database engine system initialized", LOG_LEVEL_STATE, true, true, true);
    return true;
}

bool database_engine_register(DatabaseEngineInterface* engine) {
    if (!engine || !engine_system_initialized) {
        return false;
    }

    if (engine->engine_type >= DB_ENGINE_MAX) {
        log_this(SR_DATABASE, "Invalid engine type for registration", LOG_LEVEL_ERROR, true, true, true);
        return false;
    }

    pthread_mutex_lock(&engine_registry_lock);

    if (engine_registry[engine->engine_type] != NULL) {
        log_this(SR_DATABASE, "Engine already registered for this type", LOG_LEVEL_ERROR, true, true, true);
        pthread_mutex_unlock(&engine_registry_lock);
        return false;
    }

    engine_registry[engine->engine_type] = engine;

    pthread_mutex_unlock(&engine_registry_lock);

    log_this(SR_DATABASE, "Database engine registered successfully", LOG_LEVEL_STATE, true, true, true);
    return true;
}

DatabaseEngineInterface* database_engine_get(DatabaseEngine engine_type) {
    if (engine_type >= DB_ENGINE_MAX || !engine_system_initialized) {
        return NULL;
    }

    pthread_mutex_lock(&engine_registry_lock);
    DatabaseEngineInterface* engine = engine_registry[engine_type];
    pthread_mutex_unlock(&engine_registry_lock);

    return engine;
}

DatabaseEngineInterface* database_engine_get_by_name(const char* name) {
    if (!name || !engine_system_initialized) {
        return NULL;
    }

    pthread_mutex_lock(&engine_registry_lock);

    for (int i = 0; i < DB_ENGINE_MAX; i++) {
        if (engine_registry[i] && strcmp(engine_registry[i]->name, name) == 0) {
            DatabaseEngineInterface* engine = engine_registry[i];
            pthread_mutex_unlock(&engine_registry_lock);
            return engine;
        }
    }

    pthread_mutex_unlock(&engine_registry_lock);
    return NULL;
}

/*
 * Connection Management
 */

bool database_engine_connect(DatabaseEngine engine_type, ConnectionConfig* config, DatabaseHandle** connection) {
    DatabaseEngineInterface* engine = database_engine_get(engine_type);
    if (!engine || !engine->connect || !config || !connection) {
        return false;
    }

    return engine->connect(config, connection);
}

bool database_engine_health_check(DatabaseHandle* connection) {
    if (!connection || !connection->engine_type) {
        return false;
    }

    DatabaseEngineInterface* engine = database_engine_get(connection->engine_type);
    if (!engine || !engine->health_check) {
        return false;
    }

    return engine->health_check(connection);
}

/*
 * Query Execution
 */

bool database_engine_execute(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {
    if (!connection || !request || !result) {
        return false;
    }

    DatabaseEngineInterface* engine = database_engine_get(connection->engine_type);
    if (!engine) {
        return false;
    }

    // Use prepared statement if requested and available
    if (request->use_prepared_statement && request->prepared_statement_name && engine->execute_prepared) {
        // Find prepared statement
        PreparedStatement* stmt = NULL;
        for (size_t i = 0; i < connection->prepared_statement_count; i++) {
            if (connection->prepared_statements[i] &&
                strcmp(connection->prepared_statements[i]->name, request->prepared_statement_name) == 0) {
                stmt = connection->prepared_statements[i];
                break;
            }
        }

        if (stmt) {
            return engine->execute_prepared(connection, stmt, request, result);
        }
    }

    // Fall back to regular query execution
    if (!engine->execute_query) {
        return false;
    }

    return engine->execute_query(connection, request, result);
}

/*
 * Transaction Management
 */

bool database_engine_begin_transaction(DatabaseHandle* connection, DatabaseIsolationLevel level, Transaction** transaction) {
    if (!connection || !transaction) {
        return false;
    }

    DatabaseEngineInterface* engine = database_engine_get(connection->engine_type);
    if (!engine || !engine->begin_transaction) {
        return false;
    }

    return engine->begin_transaction(connection, level, transaction);
}

bool database_engine_commit_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction) {
        return false;
    }

    DatabaseEngineInterface* engine = database_engine_get(connection->engine_type);
    if (!engine || !engine->commit_transaction) {
        return false;
    }

    return engine->commit_transaction(connection, transaction);
}

bool database_engine_rollback_transaction(DatabaseHandle* connection, Transaction* transaction) {
    if (!connection || !transaction) {
        return false;
    }

    DatabaseEngineInterface* engine = database_engine_get(connection->engine_type);
    if (!engine || !engine->rollback_transaction) {
        return false;
    }

    return engine->rollback_transaction(connection, transaction);
}

/*
 * Connection String Utilities
 */

char* database_engine_build_connection_string(DatabaseEngine engine_type, ConnectionConfig* config) {
    if (!config) {
        return NULL;
    }

    DatabaseEngineInterface* engine = database_engine_get(engine_type);
    if (!engine || !engine->get_connection_string) {
        return NULL;
    }

    return engine->get_connection_string(config);
}

bool database_engine_validate_connection_string(DatabaseEngine engine_type, const char* connection_string) {
    if (!connection_string) {
        return false;
    }

    DatabaseEngineInterface* engine = database_engine_get(engine_type);
    if (!engine || !engine->validate_connection_string) {
        return false;
    }

    return engine->validate_connection_string(connection_string);
}

/*
 * Cleanup Functions
 */

void database_engine_cleanup_connection(DatabaseHandle* connection) {
    if (!connection) {
        return;
    }

    DatabaseEngineInterface* engine = database_engine_get(connection->engine_type);
    if (engine && engine->disconnect) {
        engine->disconnect(connection);
    }

    // Clean up prepared statements
    if (connection->prepared_statements) {
        for (size_t i = 0; i < connection->prepared_statement_count; i++) {
            if (connection->prepared_statements[i]) {
                if (engine && engine->unprepare_statement) {
                    engine->unprepare_statement(connection, connection->prepared_statements[i]);
                }
                free(connection->prepared_statements[i]);
            }
        }
        free(connection->prepared_statements);
    }

    // Clean up config
    if (connection->config) {
        // Note: ConnectionConfig cleanup would be implemented here
        free(connection->config);
    }

    pthread_mutex_destroy(&connection->connection_lock);
    free(connection);
}

void database_engine_cleanup_result(QueryResult* result) {
    if (!result) {
        return;
    }

    if (result->data_json) {
        free(result->data_json);
    }

    if (result->error_message) {
        free(result->error_message);
    }

    if (result->column_names) {
        for (size_t i = 0; i < result->column_count; i++) {
            if (result->column_names[i]) {
                free(result->column_names[i]);
            }
        }
        free(result->column_names);
    }

    free(result);
}

void database_engine_cleanup_transaction(Transaction* transaction) {
    if (!transaction) {
        return;
    }

    if (transaction->transaction_id) {
        free(transaction->transaction_id);
    }

    free(transaction);
}