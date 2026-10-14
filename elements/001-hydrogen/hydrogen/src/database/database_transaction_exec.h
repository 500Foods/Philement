/*
 * Shared transactional statement executor.
 *
 * Runs an ordered list of statements inside a transaction:
 *   begin -> statements -> commit (rollback on any failure)
 *
 * Two callers share this implementation:
 *   - the runtime QueryRef transaction-block path (dbqueue worker), and
 *   - eventually the migration executors (deferred Phase 7).
 *
 * DDL handling is engine-adaptive. Engines whose DDL participates in a
 * transaction (PostgreSQL, SQLite, DB2, MSSQL) keep the whole block
 * atomic. Engines where DDL forces an early commit - MySQL/MariaDB
 * (implicit commit) and Firebird (uncommitted metadata invisible to
 * later DSQL) - get the migration path's proven pattern: commit the open
 * segment, run the DDL, open a new transaction for the remaining
 * statements. On those engines a block containing DDL is atomic per
 * segment, not end to end.
 */

#ifndef DATABASE_TRANSACTION_EXEC_H
#define DATABASE_TRANSACTION_EXEC_H

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>

/* Statement callback: invoked once per statement after execution, with
 * the statement's QueryResult (which may carry success = false). Return
 * false to abort the block (rollback). The result object is owned by the
 * executor and freed after the callback returns - copy what you keep. */
typedef bool (*TransactionStatementCallback)(size_t statement_index, const char* statement,
                                              const QueryResult* result, void* context);

/* Per-statement parameter provider. Returns the parameters_json for the
 * given statement (the executor copies it). Return NULL to fall back to
 * the executor-wide parameters_json. Lets callers bind each statement
 * only the parameters it actually references. */
typedef const char* (*TransactionParametersCallback)(size_t statement_index, const char* statement,
                                                     void* context);

typedef struct TransactionExecOutcome {
    bool success;                 // block committed
    size_t failed_statement_index; // 0-based index of the statement that failed; SIZE_MAX when none
    bool failure_at_begin;        // begin transaction failed
    bool failure_at_commit;       // commit (or post-DDL commit) failed
    size_t statements_executed;   // statements the engine accepted, including a failing one
} TransactionExecOutcome;

/*
 * Execute statements inside a transaction with the DDL policy above.
 *
 * parameters_json NULL means "{}"; timeout_seconds <= 0 means 30;
 * query_id_label NULL means "runtime_statement"; stmt_hash_prefix NULL
 * means "RTXN". isolation is always READ_COMMITTED, matching both the
 * migration apply path and the runtime queue.
 */
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
    const char* dqm_label);

/*
 * Engines whose DDL forces the transaction to close early:
 *   - MySQL / MariaDB: DDL issues an implicit COMMIT
 *   - Firebird: uncommitted metadata is invisible to later DSQL
 * PostgreSQL (and the YugabyteDB path), SQLite, DB2, and MSSQL support
 * transactional DDL, so a block stays fully atomic there.
 */
bool engine_ddl_breaks_transaction(DatabaseEngine engine_type);

/* Run one statement inside the open transaction. Internal, but declared
 * here to satisfy -Wmissing-prototypes and keep src/ free of static
 * functions. */
bool txn_exec_run_statement(DatabaseHandle* connection, const char* statement,
                            size_t index,
                            const char* parameters_json, int timeout_seconds,
                            const char* query_id_label, const char* stmt_hash_prefix,
                            bool use_prepared_statements,
                            TransactionStatementCallback statement_callback,
                            TransactionParametersCallback parameters_callback,
                            void* callback_context);

/*
 * Detect leading DDL keywords (CREATE / ALTER / DROP / RECREATE),
 * skipping whitespace and comments first. Engine-neutral home of what
 * used to be the Firebird-only helper.
 */
bool sql_statement_is_ddl(const char* sql);

#endif // DATABASE_TRANSACTION_EXEC_H
