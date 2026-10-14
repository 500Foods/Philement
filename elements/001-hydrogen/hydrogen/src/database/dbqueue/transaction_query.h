/*
 * Runtime QueryRef Transaction Blocks
 *
 * Executes a runtime QueryRef whose SQL carries transaction-block markers
 * (see transaction_blocks.h) and assembles the JSON array of per-statement
 * result sets. Unmarked SQL never reaches this file: the queue worker's
 * existing single-statement path handles it, byte for byte unchanged.
 *
 * Semantics implemented here (see TRANSACTION_PLAN_COMPLETE.md):
 *   - bare statements outside blocks: exactly today's autocommit behavior
 *   - transaction blocks: one transaction per block, via the shared
 *     executor in database_transaction_exec.c (engine-adaptive DDL)
 *   - result payload: data_json is an array of per-statement entries;
 *     top-level row_count / affected_rows are totals
 *   - failures: everything known comes back - entries collected so far,
 *     the failing statement and block, and the engine's message
 *   - parameters: validated globally by the conduit layer (union of all
 *     names); each statement is bound only the names it references
 */

#ifndef DATABASE_TRANSACTION_QUERY_H
#define DATABASE_TRANSACTION_QUERY_H

// Project includes
#include <src/hydrogen.h>
#include <jansson.h>

/*
 * Execute a marked runtime query. Returns true and sets *out_result when
 * the query succeeded (result->success true); returns false with
 * *out_result set (or NULL on allocation failure) when parsing or any
 * statement failed. The caller owns and must clean up the result with
 * database_engine_cleanup_result.
 */
bool database_queue_execute_query_with_blocks(DatabaseHandle* connection, QueryRequest* request,
                                              QueryResult** out_result, const char* dqm_label);

/*
 * Internals. Declared here (not static) because src/ forbids static
 * functions and the trial build runs with -Wmissing-prototypes.
 */

/*
 * Per-query collection state. One entry per executed statement is
 * appended to `entries`; totals feed the top-level QueryResult fields.
 */
typedef struct BlockCollector {
    json_t* entries;           // array of per-statement entries
    json_t* parameters_root;   // parsed request->parameters_json, may be NULL
    size_t current_block;      // 1-based block number for entries
    size_t total_rows;
    size_t total_columns;
    long long total_affected;
    long long total_time_ms;
    char filter_buffer[8192];  // per-statement parameters JSON
    bool filter_failed;        // allocation failure while filtering
} BlockCollector;

BlockCollector* txn_collector_create(const char* parameters_json);
void txn_collector_free(BlockCollector* collector);

bool txn_collect_entry(BlockCollector* collector, size_t statement_index, const QueryResult* result);
bool txn_statement_callback(size_t statement_index, const char* statement,
                            const QueryResult* result, void* context);
const char* txn_parameters_callback(size_t statement_index, const char* statement, void* context);
char* txn_filtered_parameters(BlockCollector* collector, const char* statement);
QueryResult* txn_build_result(BlockCollector* collector, bool success, const char* error_message);
bool txn_execute_bare_statement(DatabaseHandle* connection, BlockCollector* collector,
                                const char* statement, QueryRequest* request,
                                QueryResult** statement_result, const char* dqm_label);

#endif // DATABASE_TRANSACTION_QUERY_H
