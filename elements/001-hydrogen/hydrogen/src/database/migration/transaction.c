/*
 * Database Migration Transaction Handling
 *
 * Provides unified transaction handling for all database engines.
 * Supports PostgreSQL, MySQL, SQLite, and DB2 with explicit transaction control.
 */

// Project includes
#include <src/hydrogen.h>

// Unity test mode - enable mocks BEFORE including real headers
#ifdef UNITY_TEST_MODE
#ifdef USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>
#endif
#ifdef USE_MOCK_DATABASE_ENGINE
#include <unity/mocks/mock_database_engine.h>
#endif
#ifdef USE_MOCK_DB2_TRANSACTION
#include <unity/mocks/mock_db2_transaction.h>
#endif
#endif

#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/utils/utils_hash.h>

// Local includes
#include "migration.h"

// DB2 includes for transaction control
#include <src/database/db2/types.h>
#include <src/database/db2/connection.h>
#include <src/database/db2/transaction.h>

/*
 * Parse multi-statement SQL into individual statements
 */
bool parse_sql_statements(const char* sql_result, size_t sql_length, char*** statements,
                         size_t* statement_count, size_t* statements_capacity,
                         const char* dqm_label) {
    (void)sql_length; // Suppress unused parameter warning

    if (!sql_result || sql_length == 0) {
        log_this(dqm_label, "SQL result is NULL or empty", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Parse the SQL into individual statements using the -- QUERY DELIMITER
    char* sql_copy = strdup(sql_result);
    if (!sql_copy) {
        log_this(dqm_label, "Failed to allocate memory for SQL parsing", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Split on "-- QUERY DELIMITER\n" manually (strtok doesn't work well with multi-char delimiters)
    const char* delimiter = "-- QUERY DELIMITER\n";
    size_t delimiter_len = strlen(delimiter);
    char* sql_ptr = sql_copy;
    bool parse_success = true;

    while (*sql_ptr != '\0') {
        // Find the next delimiter
        char* delimiter_pos = strstr(sql_ptr, delimiter);

        // If delimiter found, temporarily null-terminate it
        if (delimiter_pos) {
            *delimiter_pos = '\0';
        }

        // Process the current statement
        char* stmt = sql_ptr;

        // Trim whitespace
        while (*stmt && isspace((unsigned char)*stmt)) stmt++;
        char* end = stmt + strlen(stmt) - 1;
        while (end > stmt && isspace((unsigned char)*end)) {
            *end = '\0';
            end--;
        }

        // Skip empty statements
        if (strlen(stmt) > 0) {
            // Add to statements array
            if (*statement_count >= *statements_capacity) {
                *statements_capacity = *statements_capacity == 0 ? 10 : *statements_capacity * 2;
                char** new_statements = realloc(*statements, *statements_capacity * sizeof(char*));
                if (!new_statements) {
                    log_this(dqm_label, "Failed to allocate memory for statements array", LOG_LEVEL_ERROR, 0);
                    parse_success = false;
                    break;
                }
                *statements = new_statements;
            }
            (*statements)[*statement_count] = strdup(stmt);
            (*statement_count)++;
        }

        // Move to next statement
        if (delimiter_pos) {
            // Restore the delimiter and move past it
            *delimiter_pos = '-';  // Restore first char of delimiter
            sql_ptr = delimiter_pos + delimiter_len;
        } else {
            // No more delimiters, we're done
            break;
        }
    }

    free(sql_copy);
    return parse_success;
}

/*
 * Execute migration statements for DB2 with explicit transaction control
 * Updated to use database_engine functions for consistency with other engines
 */
bool execute_db2_migration(DatabaseHandle* connection, char** statements, size_t statement_count,
                          const char* migration_file, const char* dqm_label) {
    // Execute all statements within a transaction using database_engine functions
    Transaction* db2_transaction = NULL;
    if (!database_engine_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &db2_transaction)) {
        log_this(dqm_label, "Failed to begin DB2 transaction for migration %s", LOG_LEVEL_ERROR, 1, migration_file);
        return false;
    }

    log_this(dqm_label, "Started DB2 transaction for migration %s (%zu statements)", LOG_LEVEL_TRACE, 2, migration_file, statement_count);

    bool transaction_success = true;

    // Execute all statements
    for (size_t j = 0; j < statement_count && transaction_success; j++) {
        QueryRequest* stmt_request = calloc(1, sizeof(QueryRequest));
        if (!stmt_request) {
            log_this(dqm_label, "Failed to allocate statement request", LOG_LEVEL_ERROR, 0);
            transaction_success = false;
            break;
        }

        // Generate hash for prepared statement caching
        char stmt_hash[64];  // Sufficient buffer for hash (prefix + 16 hex chars + null)
        get_stmt_hash("MPSC", statements[j], 16, stmt_hash);

        log_this(dqm_label, "Statement %zu using prepared statement hash: %s", LOG_LEVEL_TRACE, 2, j + 1, stmt_hash);
        log_this(dqm_label, "Statement %zu SQL: %.100s%s", LOG_LEVEL_TRACE, 3, j + 1, statements[j], strlen(statements[j]) > 100 ? "..." : "");

        stmt_request->query_id = strdup("migration_statement");
        stmt_request->sql_template = strdup(statements[j]);
        stmt_request->parameters_json = strdup("{}");
        stmt_request->timeout_seconds = 30;
        stmt_request->isolation_level = DB_ISOLATION_READ_COMMITTED;
        stmt_request->use_prepared_statement = true;
        stmt_request->prepared_statement_name = strdup(stmt_hash);

        QueryResult* stmt_result = NULL;
        bool stmt_success = database_engine_execute(connection, stmt_request, &stmt_result);

        // Clean up request
        if (stmt_request->query_id) free(stmt_request->query_id);
        if (stmt_request->sql_template) free(stmt_request->sql_template);
        if (stmt_request->parameters_json) free(stmt_request->parameters_json);
        if (stmt_request->prepared_statement_name) free(stmt_request->prepared_statement_name);
        free(stmt_request);

        if (stmt_success && stmt_result && stmt_result->success) {
            log_this(dqm_label, "Statement %zu executed successfully (hash: %s): affected %lld rows", LOG_LEVEL_TRACE, 3, j + 1, stmt_hash, stmt_result->affected_rows);
        } else {
            log_this(dqm_label, "Statement %zu failed (hash: %s)", LOG_LEVEL_ERROR, 2, j + 1, stmt_hash);
            transaction_success = false;
        }

        if (stmt_result) {
            database_engine_cleanup_result(stmt_result);
        }
    }

    // Commit or rollback based on success
    if (transaction_success) {
        // Commit the transaction
        if (!database_engine_commit_transaction(connection, db2_transaction)) {
            log_this(dqm_label, "Failed to commit migration %s", LOG_LEVEL_ERROR, 1, migration_file);
            transaction_success = false;
        } else {
            log_this(dqm_label, "Migration %s committed successfully", LOG_LEVEL_TRACE, 1, migration_file);
        }
    } else {
        // Rollback the transaction
        if (!database_engine_rollback_transaction(connection, db2_transaction)) {
            log_this(dqm_label, "Failed to rollback migration %s", LOG_LEVEL_ERROR, 1, migration_file);
        } else {
            log_this(dqm_label, "Migration %s rolled back due to errors", LOG_LEVEL_TRACE, 1, migration_file);
        }
    }

    // Clean up transaction structure
    database_engine_cleanup_transaction(db2_transaction);

    return transaction_success;
}

/*
 * Execute migration statements for PostgreSQL with explicit transaction control using PostgreSQL transaction functions
 */
bool execute_postgresql_migration(DatabaseHandle* connection, char** statements, size_t statement_count,
                                 const char* migration_file, const char* dqm_label) {
    // Execute all statements within a transaction using proper PostgreSQL transaction functions
    Transaction* pg_transaction = NULL;
    if (!database_engine_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &pg_transaction)) {
        log_this(dqm_label, "Failed to begin PostgreSQL transaction for migration %s", LOG_LEVEL_ERROR, 1, migration_file);
        return false;
    }

    log_this(dqm_label, "Started PostgreSQL transaction for migration %s (%zu statements)", LOG_LEVEL_TRACE, 2, migration_file, statement_count);

    bool transaction_success = true;

    // Execute all statements
    for (size_t j = 0; j < statement_count && transaction_success; j++) {
        QueryRequest* stmt_request = calloc(1, sizeof(QueryRequest));
        if (!stmt_request) {
            log_this(dqm_label, "Failed to allocate statement request", LOG_LEVEL_ERROR, 0);
            transaction_success = false;
            break;
        }

        // Generate hash for prepared statement caching
        char stmt_hash[64];  // Sufficient buffer for hash (prefix + 16 hex chars + null)
        get_stmt_hash("MPSC", statements[j], 16, stmt_hash);

        log_this(dqm_label, "Statement %zu using prepared statement hash: %s", LOG_LEVEL_TRACE, 2, j + 1, stmt_hash);
        log_this(dqm_label, "Statement %zu SQL: %.100s%s", LOG_LEVEL_TRACE, 3, j + 1, statements[j], strlen(statements[j]) > 100 ? "..." : "");

        stmt_request->query_id = strdup("migration_statement");
        stmt_request->sql_template = strdup(statements[j]);
        stmt_request->parameters_json = strdup("{}");
        stmt_request->timeout_seconds = 30;
        stmt_request->isolation_level = DB_ISOLATION_READ_COMMITTED;
        stmt_request->use_prepared_statement = true;
        stmt_request->prepared_statement_name = strdup(stmt_hash);

        QueryResult* stmt_result = NULL;
        bool stmt_success = database_engine_execute(connection, stmt_request, &stmt_result);

        // Clean up request
        if (stmt_request->query_id) free(stmt_request->query_id);
        if (stmt_request->sql_template) free(stmt_request->sql_template);
        if (stmt_request->parameters_json) free(stmt_request->parameters_json);
        if (stmt_request->prepared_statement_name) free(stmt_request->prepared_statement_name);
        free(stmt_request);

        if (stmt_success && stmt_result && stmt_result->success) {
            log_this(dqm_label, "Statement %zu executed successfully (hash: %s): affected %lld rows", LOG_LEVEL_TRACE, 3, j + 1, stmt_hash, stmt_result->affected_rows);
        } else {
            log_this(dqm_label, "Statement %zu failed (hash: %s)", LOG_LEVEL_ERROR, 2, j + 1, stmt_hash);
            transaction_success = false;
        }

        if (stmt_result) {
            database_engine_cleanup_result(stmt_result);
        }
    }

    // Commit or rollback based on success
    if (transaction_success) {
        // Commit the transaction
        if (!database_engine_commit_transaction(connection, pg_transaction)) {
            log_this(dqm_label, "Failed to commit migration %s", LOG_LEVEL_ERROR, 1, migration_file);
            transaction_success = false;
        } else {
            log_this(dqm_label, "Migration %s committed successfully", LOG_LEVEL_TRACE, 1, migration_file);
        }
    } else {
        // Rollback the transaction
        if (!database_engine_rollback_transaction(connection, pg_transaction)) {
            log_this(dqm_label, "Failed to rollback migration %s", LOG_LEVEL_ERROR, 1, migration_file);
        } else {
            log_this(dqm_label, "Migration %s rolled back due to errors", LOG_LEVEL_TRACE, 1, migration_file);
        }
    }

    // Clean up transaction structure
    database_engine_cleanup_transaction(pg_transaction);

    return transaction_success;
}

/*
 * Execute migration statements for MySQL with explicit transaction control using MySQL transaction functions
 */
bool execute_mysql_migration(DatabaseHandle* connection, char** statements, size_t statement_count,
                            const char* migration_file, const char* dqm_label) {
    // Execute all statements within a transaction using proper MySQL transaction functions
    Transaction* mysql_transaction = NULL;
    if (!database_engine_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &mysql_transaction)) {
        log_this(dqm_label, "Failed to begin MySQL transaction for migration %s", LOG_LEVEL_ERROR, 1, migration_file);
        return false;
    }

    log_this(dqm_label, "Started MySQL transaction for migration %s (%zu statements)", LOG_LEVEL_TRACE, 2, migration_file, statement_count);

    bool transaction_success = true;

    // Execute all statements
    for (size_t j = 0; j < statement_count && transaction_success; j++) {
        QueryRequest* stmt_request = calloc(1, sizeof(QueryRequest));
        if (!stmt_request) {
            log_this(dqm_label, "Failed to allocate statement request", LOG_LEVEL_ERROR, 0);
            transaction_success = false;
            break;
        }

        // Generate hash for prepared statement caching
        char stmt_hash[64];  // Sufficient buffer for hash (prefix + 16 hex chars + null)
        get_stmt_hash("MPSC", statements[j], 16, stmt_hash);

        log_this(dqm_label, "Statement %zu using prepared statement hash: %s", LOG_LEVEL_TRACE, 2, j + 1, stmt_hash);
        log_this(dqm_label, "Statement %zu SQL: %.100s%s", LOG_LEVEL_TRACE, 3, j + 1, statements[j], strlen(statements[j]) > 100 ? "..." : "");

        stmt_request->query_id = strdup("migration_statement");
        stmt_request->sql_template = strdup(statements[j]);
        stmt_request->parameters_json = strdup("{}");
        stmt_request->timeout_seconds = 30;
        stmt_request->isolation_level = DB_ISOLATION_READ_COMMITTED;
        stmt_request->use_prepared_statement = true;
        stmt_request->prepared_statement_name = strdup(stmt_hash);

        QueryResult* stmt_result = NULL;
        bool stmt_success = database_engine_execute(connection, stmt_request, &stmt_result);

        // Clean up request
        if (stmt_request->query_id) free(stmt_request->query_id);
        if (stmt_request->sql_template) free(stmt_request->sql_template);
        if (stmt_request->parameters_json) free(stmt_request->parameters_json);
        if (stmt_request->prepared_statement_name) free(stmt_request->prepared_statement_name);
        free(stmt_request);

        if (stmt_success && stmt_result && stmt_result->success) {
            log_this(dqm_label, "Statement %zu executed successfully (hash: %s): affected %lld rows", LOG_LEVEL_TRACE, 3, j + 1, stmt_hash, stmt_result->affected_rows);
        } else {
            log_this(dqm_label, "Statement %zu failed (hash: %s)", LOG_LEVEL_ERROR, 2, j + 1, stmt_hash);
            transaction_success = false;
        }

        if (stmt_result) {
            database_engine_cleanup_result(stmt_result);
        }
    }

    // Commit or rollback based on success
    if (transaction_success) {
        // Commit the transaction
        if (!database_engine_commit_transaction(connection, mysql_transaction)) {
            log_this(dqm_label, "Failed to commit migration %s", LOG_LEVEL_ERROR, 1, migration_file);
            transaction_success = false;
        } else {
            log_this(dqm_label, "Migration %s committed successfully", LOG_LEVEL_TRACE, 1, migration_file);
        }
    } else {
        // Rollback the transaction
        if (!database_engine_rollback_transaction(connection, mysql_transaction)) {
            log_this(dqm_label, "Failed to rollback migration %s", LOG_LEVEL_ERROR, 1, migration_file);
        } else {
            log_this(dqm_label, "Migration %s rolled back due to errors", LOG_LEVEL_TRACE, 1, migration_file);
        }
    }

    // Clean up transaction structure
    database_engine_cleanup_transaction(mysql_transaction);

    return transaction_success;
}

/*
 * Execute migration statements for SQLite with explicit transaction control using SQLite transaction functions
 */
bool execute_sqlite_migration(DatabaseHandle* connection, char** statements, size_t statement_count,
                             const char* migration_file, const char* dqm_label) {
    // Execute all statements within a transaction using proper SQLite transaction functions
    Transaction* sqlite_transaction = NULL;
    if (!database_engine_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &sqlite_transaction)) {
        log_this(dqm_label, "Failed to begin SQLite transaction for migration %s", LOG_LEVEL_ERROR, 1, migration_file);
        return false;
    }

    log_this(dqm_label, "Started SQLite transaction for migration %s (%zu statements)", LOG_LEVEL_TRACE, 2, migration_file, statement_count);

    bool transaction_success = true;

    // Execute all statements
    for (size_t j = 0; j < statement_count && transaction_success; j++) {
        QueryRequest* stmt_request = calloc(1, sizeof(QueryRequest));
        if (!stmt_request) {
            log_this(dqm_label, "Failed to allocate statement request", LOG_LEVEL_ERROR, 0);
            transaction_success = false;
            break;
        }

        // Generate hash for prepared statement caching
        char stmt_hash[64];  // Sufficient buffer for hash (prefix + 16 hex chars + null)
        get_stmt_hash("MPSC", statements[j], 16, stmt_hash);

        log_this(dqm_label, "Statement %zu using prepared statement hash: %s", LOG_LEVEL_TRACE, 2, j + 1, stmt_hash);
        log_this(dqm_label, "Statement %zu SQL: %.100s%s", LOG_LEVEL_TRACE, 3, j + 1, statements[j], strlen(statements[j]) > 100 ? "..." : "");

        stmt_request->query_id = strdup("migration_statement");
        stmt_request->sql_template = strdup(statements[j]);
        stmt_request->parameters_json = strdup("{}");
        stmt_request->timeout_seconds = 30;
        stmt_request->isolation_level = DB_ISOLATION_READ_COMMITTED;
        stmt_request->use_prepared_statement = true;
        stmt_request->prepared_statement_name = strdup(stmt_hash);

        QueryResult* stmt_result = NULL;
        bool stmt_success = database_engine_execute(connection, stmt_request, &stmt_result);

        // Clean up request
        if (stmt_request->query_id) free(stmt_request->query_id);
        if (stmt_request->sql_template) free(stmt_request->sql_template);
        if (stmt_request->parameters_json) free(stmt_request->parameters_json);
        if (stmt_request->prepared_statement_name) free(stmt_request->prepared_statement_name);
        free(stmt_request);

        if (stmt_success && stmt_result && stmt_result->success) {
            log_this(dqm_label, "Statement %zu executed successfully (hash: %s): affected %lld rows", LOG_LEVEL_TRACE, 3, j + 1, stmt_hash, stmt_result->affected_rows);
        } else {
            log_this(dqm_label, "Statement %zu failed (hash: %s)", LOG_LEVEL_ERROR, 2, j + 1, stmt_hash);
            transaction_success = false;
        }

        if (stmt_result) {
            database_engine_cleanup_result(stmt_result);
        }
    }

    // Commit or rollback based on success
    if (transaction_success) {
        // Commit the transaction
        if (!database_engine_commit_transaction(connection, sqlite_transaction)) {
            log_this(dqm_label, "Failed to commit migration %s", LOG_LEVEL_ERROR, 1, migration_file);
            transaction_success = false;
        } else {
            log_this(dqm_label, "Migration %s committed successfully", LOG_LEVEL_TRACE, 1, migration_file);
        }
    } else {
        // Rollback the transaction
        if (!database_engine_rollback_transaction(connection, sqlite_transaction)) {
            log_this(dqm_label, "Failed to rollback migration %s", LOG_LEVEL_ERROR, 1, migration_file);
        } else {
            log_this(dqm_label, "Migration %s rolled back due to errors", LOG_LEVEL_TRACE, 1, migration_file);
        }
    }

    // Clean up transaction structure
    database_engine_cleanup_transaction(sqlite_transaction);

    return transaction_success;
}

/*
 * Execute migration SQL as a transaction for any database engine
 */
bool execute_transaction(DatabaseHandle* connection, const char* sql_result,
                                          size_t sql_length, const char* migration_file,
                                          DatabaseEngine engine_type, const char* dqm_label) {
    if (!sql_result || sql_length == 0) {
        log_this(dqm_label, "No SQL generated for migration: %s", LOG_LEVEL_TRACE, 1, migration_file);
        return false;
    }

    log_this(dqm_label, "Executing migration %s as transaction", LOG_LEVEL_TRACE, 1, migration_file);

    // Parse the SQL into individual statements
    char** statements = NULL;
    size_t statement_count = 0;
    size_t statements_capacity = 0;

    if (!parse_sql_statements(sql_result, sql_length, &statements, &statement_count, &statements_capacity, dqm_label)) {
        return false;
    }

    if (statement_count == 0) {
        log_this(dqm_label, "No valid statements found in migration: %s", LOG_LEVEL_TRACE, 1, migration_file);
        free(statements);
        return false;
    }

    bool success = false;

    // Execute based on database engine
    switch (engine_type) {
        case DB_ENGINE_POSTGRESQL:
            success = execute_postgresql_migration(connection, statements, statement_count, migration_file, dqm_label);
            break;
        case DB_ENGINE_MYSQL:
            success = execute_mysql_migration(connection, statements, statement_count, migration_file, dqm_label);
            break;
        case DB_ENGINE_SQLITE:
            success = execute_sqlite_migration(connection, statements, statement_count, migration_file, dqm_label);
            break;
        case DB_ENGINE_DB2:
            success = execute_db2_migration(connection, statements, statement_count, migration_file, dqm_label);
            break;
        case DB_ENGINE_AI:
        case DB_ENGINE_MAX:
        default:
            log_this(dqm_label, "Unsupported database engine for migration: %d", LOG_LEVEL_ERROR, 1, engine_type);
            success = false;
            break;
    }

    // Cleanup statements
    for (size_t j = 0; j < statement_count; j++) {
        free(statements[j]);
    }
    free(statements);

    if (!success) {
        log_this(dqm_label, "Migration %s failed - transaction rolled back", LOG_LEVEL_TRACE, 1, migration_file);
    } else {
        log_this(dqm_label, "Migration %s LOAD was successful", LOG_LEVEL_STATE, 1, migration_file);
    }

    return success;
}