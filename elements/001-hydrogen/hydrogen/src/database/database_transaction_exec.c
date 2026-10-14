/*
 * Shared transactional statement executor.
 * See database_transaction_exec.h for the contract and the DDL policy.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/database_engine.h>
#include <src/utils/utils_hash.h>

// Local includes
#include "database_transaction_exec.h"

bool engine_ddl_breaks_transaction(DatabaseEngine engine_type) {
    switch (engine_type) {
        case DB_ENGINE_MYSQL:
        case DB_ENGINE_MARIADB:
        case DB_ENGINE_FIREBIRD:
            return true;
        case DB_ENGINE_POSTGRESQL:
        case DB_ENGINE_SQLITE:
        case DB_ENGINE_DB2:
        case DB_ENGINE_MSSQL:
        case DB_ENGINE_AI:
        case DB_ENGINE_MAX:
            return false;
    }
    return false;
}

bool sql_statement_is_ddl(const char* sql) {
    if (!sql) {
        return false;
    }

    const char* p = sql;
    for (;;) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
            p++;
        }
        if (p[0] == '-' && p[1] == '-') {
            while (*p && *p != '\n') {
                p++;
            }
            continue;
        }
        if (p[0] == '/' && p[1] == '*') {
            p += 2;
            while (*p && !(p[0] == '*' && p[1] == '/')) {
                p++;
            }
            if (*p) {
                p += 2;
            }
            continue;
        }
        break;
    }

    static const char* const kws[] = { "CREATE", "ALTER", "DROP", "RECREATE", NULL };
    for (size_t i = 0; kws[i]; i++) {
        size_t n = strlen(kws[i]);
        if (strncasecmp(p, kws[i], n) == 0) {
            unsigned char c = (unsigned char)p[n];
            if (c == '\0' || (!isalnum(c) && c != '_')) {
                return true;
            }
        }
    }
    return false;
}

/*
 * Build and run one statement request. Returns the executor's verdict:
 * true when the engine accepted the statement and any callback agreed.
 * The result object (when produced) is handed to the callback and freed
 * here.
 */
bool txn_exec_run_statement(DatabaseHandle* connection, const char* statement,
                            size_t index,
                            const char* parameters_json, int timeout_seconds,
                            const char* query_id_label, const char* stmt_hash_prefix,
                            bool use_prepared_statements,
                            TransactionStatementCallback statement_callback,
                            TransactionParametersCallback parameters_callback,
                            void* callback_context) {
    QueryRequest* request = calloc(1, sizeof(QueryRequest));
    if (!request) {
        return false;
    }

    request->query_id = strdup(query_id_label);
    request->sql_template = strdup(statement);

    const char* statement_parameters = parameters_json;
    char* provided_parameters = NULL;
    if (parameters_callback) {
        const char* per_statement = parameters_callback(index, statement, callback_context);
        if (per_statement) {
            provided_parameters = strdup(per_statement);
            statement_parameters = provided_parameters;
        }
    }
    request->parameters_json = strdup(statement_parameters ? statement_parameters : "{}");
    request->timeout_seconds = timeout_seconds;
    request->isolation_level = DB_ISOLATION_READ_COMMITTED;
    request->use_prepared_statement = use_prepared_statements;

    if (use_prepared_statements) {
        char stmt_hash[64];
        get_stmt_hash(stmt_hash_prefix, statement, 16, stmt_hash);
        request->prepared_statement_name = strdup(stmt_hash);
    }

    bool request_ok = request->query_id && request->sql_template && request->parameters_json &&
                      (!use_prepared_statements || request->prepared_statement_name);

    QueryResult* result = NULL;
    bool executed = false;
    if (request_ok) {
        executed = database_engine_execute(connection, request, &result);
    }

    if (request->query_id) free(request->query_id);
    if (request->sql_template) free(request->sql_template);
    if (request->parameters_json) free(request->parameters_json);
    if (request->prepared_statement_name) free(request->prepared_statement_name);
    free(request);
    free(provided_parameters);

    if (!request_ok) {
        if (result) {
            database_engine_cleanup_result(result);
        }
        return false;
    }

    bool accepted = executed && result && result->success;
    if (statement_callback) {
        if (!statement_callback(index, statement, result, callback_context)) {
            if (result) {
                database_engine_cleanup_result(result);
            }
            return false;
        }
    }

    if (result) {
        database_engine_cleanup_result(result);
    }
    return accepted;
}

bool database_execute_statements_in_transaction(
    DatabaseHandle* connection,
    char** statements, size_t statement_count,
    const char* parameters_json,
    int timeout_seconds,
    const char* query_id_label,
    const char* stmt_hash_prefix,
    bool use_prepared_statements,
    TransactionStatementCallback statement_callback,
    TransactionParametersCallback parameters_callback,
    void* callback_context,
    TransactionExecOutcome* outcome,
    const char* dqm_label) {

    if (outcome) {
        memset(outcome, 0, sizeof(*outcome));
        outcome->failed_statement_index = (size_t)-1;
    }

    if (!connection) {
        return false;
    }

    if (!statements || statement_count == 0) {
        // Nothing to run: a committed no-op.
        if (outcome) {
            outcome->success = true;
        }
        return true;
    }

    const char* label = dqm_label ? dqm_label : "TXN";
    const char* qid = query_id_label ? query_id_label : "runtime_statement";
    const char* hash_prefix = stmt_hash_prefix ? stmt_hash_prefix : "RTXN";
    const char* params = parameters_json ? parameters_json : "{}";
    int timeout = timeout_seconds > 0 ? timeout_seconds : 30;
    bool ddl_breaks = engine_ddl_breaks_transaction(connection->engine_type);

    Transaction* transaction = NULL;
    if (!database_engine_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction)) {
        log_this(label, "Failed to begin transaction for statement block", LOG_LEVEL_ERROR, 0);
        if (outcome) {
            outcome->failure_at_begin = true;
        }
        return false;
    }

    bool success = true;
    size_t statements_executed = 0;
    for (size_t i = 0; i < statement_count; i++) {
        const char* statement = statements[i];
        if (!statement) {
            success = false;
            if (outcome) {
                outcome->failed_statement_index = i;
            }
            break;
        }

        // DDL on an engine that cannot keep DDL inside a transaction:
        // commit the open segment, run the DDL in autocommit, and open a
        // new transaction for whatever follows (the migration path's
        // Firebird pattern, applied to MySQL/MariaDB as well).
        if (ddl_breaks && sql_statement_is_ddl(statement)) {
            if (transaction) {
                if (!database_engine_commit_transaction(connection, transaction)) {
                    log_this(label, "Failed to commit transaction before DDL statement %zu", LOG_LEVEL_ERROR, 1, i + 1);
                    database_engine_cleanup_transaction(transaction);
                    if (outcome) {
                        outcome->failure_at_commit = true;
                        outcome->failed_statement_index = i;
                        outcome->statements_executed = statements_executed;
                    }
                    return false;
                }
                database_engine_cleanup_transaction(transaction);
                transaction = NULL;
            }

            statements_executed++;
            if (!txn_exec_run_statement(connection, statement, i, params, timeout,
                                        qid, hash_prefix, use_prepared_statements, statement_callback,
                                        parameters_callback, callback_context)) {
                if (outcome) {
                    outcome->failed_statement_index = i;
                    outcome->statements_executed = statements_executed;
                }
                return false;
            }

            if (i + 1 < statement_count) {
                if (!database_engine_begin_transaction(connection, DB_ISOLATION_READ_COMMITTED, &transaction)) {
                    log_this(label, "Failed to reopen transaction after DDL statement %zu", LOG_LEVEL_ERROR, 1, i + 1);
                    if (outcome) {
                        outcome->failure_at_begin = true;
                        outcome->failed_statement_index = i + 1;
                        outcome->statements_executed = statements_executed;
                    }
                    return false;
                }
            }
            continue;
        }

        statements_executed++;
        if (!txn_exec_run_statement(connection, statement, i, params, timeout,
                                    qid, hash_prefix, use_prepared_statements, statement_callback,
                                    parameters_callback, callback_context)) {
            success = false;
            if (outcome) {
                outcome->failed_statement_index = i;
                outcome->statements_executed = statements_executed;
            }
            break;
        }
    }

    if (!success && transaction) {
        if (!database_engine_rollback_transaction(connection, transaction)) {
            log_this(label, "Failed to rollback transaction after statement failure", LOG_LEVEL_ERROR, 0);
        }
        database_engine_cleanup_transaction(transaction);
        return false;
    }

    if (transaction) {
        if (!database_engine_commit_transaction(connection, transaction)) {
            log_this(label, "Failed to commit transaction for statement block", LOG_LEVEL_ERROR, 0);
            database_engine_rollback_transaction(connection, transaction);
            database_engine_cleanup_transaction(transaction);
            if (outcome) {
                outcome->failure_at_commit = true;
                outcome->statements_executed = statements_executed;
            }
            return false;
        }
        database_engine_cleanup_transaction(transaction);
    }

    if (outcome) {
        outcome->success = true;
        outcome->statements_executed = statements_executed;
    }
    return true;
}
