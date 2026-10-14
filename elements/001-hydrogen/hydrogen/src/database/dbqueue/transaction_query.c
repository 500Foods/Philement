/*
 * Runtime QueryRef Transaction Blocks
 * See transaction_query.h and TRANSACTION_PLAN.md for the contract.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/database_engine.h>
#include <src/database/database_transaction_exec.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/dbqueue/transaction_blocks.h>
#include <src/api/conduit/helpers/param_proc_helpers.h>

// Local includes
#include "transaction_query.h"

/*
 * Create the per-query collection state. parameters_json may be NULL.
 */
BlockCollector* txn_collector_create(const char* parameters_json) {
    BlockCollector* collector = calloc(1, sizeof(BlockCollector));
    if (!collector) {
        return NULL;
    }

    collector->entries = json_array();
    if (!collector->entries) {
        free(collector);
        return NULL;
    }

    if (parameters_json && strlen(parameters_json) > 0) {
        json_error_t parse_error;
        collector->parameters_root = json_loads(parameters_json, 0, &parse_error);
    }
    return collector;
}

void txn_collector_free(BlockCollector* collector) {
    if (!collector) {
        return;
    }
    if (collector->parameters_root) {
        json_decref(collector->parameters_root);
    }
    if (collector->entries) {
        json_decref(collector->entries);
    }
    free(collector);
}

/*
 * Append one statement's entry to the collection.
 */
bool txn_collect_entry(BlockCollector* collector, size_t statement_index,
                       const QueryResult* result) {
    json_t* entry = json_object();
    if (!entry) {
        return false;
    }

    json_object_set_new(entry, "statement", json_integer((json_int_t)(statement_index + 1)));
    json_object_set_new(entry, "block", json_integer((json_int_t)collector->current_block));

    json_t* rows = NULL;
    if (result && result->data_json) {
        json_error_t parse_error;
        rows = json_loads(result->data_json, 0, &parse_error);
    }
    if (!rows || !json_is_array(rows)) {
        if (rows) {
            json_decref(rows);
        }
        rows = json_array();
    }
    json_object_set_new(entry, "rows", rows);

    long long affected = (result && result->success) ? result->affected_rows : 0;
    json_object_set_new(entry, "affected", json_integer((json_int_t)affected));
    json_object_set_new(entry, "success", json_boolean(result && result->success));

    if (result && !result->success && result->error_message) {
        json_object_set_new(entry, "error", json_string(result->error_message));
    } else {
        json_object_set_new(entry, "error", json_null());
    }

    if (json_array_append_new(collector->entries, entry) != 0) {
        return false;
    }

    if (result) {
        collector->total_rows += result->row_count;
        if (collector->total_columns == 0) {
            collector->total_columns = result->column_count;
        }
        collector->total_affected += result->affected_rows;
        collector->total_time_ms += (long long)result->execution_time_ms;
    }
    (void)statement_index;
    return true;
}

/*
 * Statement callback for transaction blocks: record every statement's
 * result (including a failing one, so the payload can name it).
 */
bool txn_statement_callback(size_t statement_index, const char* statement,
                            const QueryResult* result, void* context) {
    (void)statement;
    BlockCollector* collector = (BlockCollector*)context;
    return txn_collect_entry(collector, statement_index, result);
}

/*
 * Per-statement parameter filter: keep only the names this statement
 * references. Global missing/unused validation already ran against the
 * union of all names, so dropping names a statement does not reference
 * only prevents "too many parameters" rejections at the engine.
 *
 * parameters_json is Hydrogen's typed shape:
 *   {"INTEGER": {"SHOULD_FAIL": 1}, "STRING": {"RUN_LABEL": "x"}}
 * (see parse_typed_parameters in database_params.c), so the filter keeps
 * the type sections and prunes unreferenced names inside each one.
 */
const char* txn_parameters_callback(size_t statement_index, const char* statement, void* context) {
    (void)statement_index;
    BlockCollector* collector = (BlockCollector*)context;
    collector->filter_buffer[0] = '\0';

    if (!collector->parameters_root) {
        // No parameters supplied at all.
        snprintf(collector->filter_buffer, sizeof(collector->filter_buffer), "{}");
        return collector->filter_buffer;
    }

    size_t required_count = 0;
    char** required = extract_required_parameters(statement, &required_count);

    json_t* filtered = json_object();
    if (!filtered) {
        if (required) {
            for (size_t i = 0; i < required_count; i++) {
                free(required[i]);
            }
            free(required);
        }
        collector->filter_failed = true;
        snprintf(collector->filter_buffer, sizeof(collector->filter_buffer), "{}");
        return collector->filter_buffer;
    }

    // Walk the supplied type sections, keeping only referenced names.
    const char* type_key;
    json_t* type_value;
    json_object_foreach(collector->parameters_root, type_key, type_value) {
        if (!json_is_object(type_value)) {
            continue;
        }
        json_t* section = json_object();
        if (!section) {
            json_decref(filtered);
            for (size_t i = 0; i < required_count; i++) {
                free(required[i]);
            }
            free(required);
            collector->filter_failed = true;
            snprintf(collector->filter_buffer, sizeof(collector->filter_buffer), "{}");
            return collector->filter_buffer;
        }
        const char* name;
        json_t* value;
        json_object_foreach(type_value, name, value) {
            for (size_t i = 0; i < required_count; i++) {
                if (strcmp(name, required[i]) == 0) {
                    json_object_set(section, name, value);
                    break;
                }
            }
        }
        if (json_object_size(section) > 0) {
            json_object_set_new(filtered, type_key, section);
        } else {
            json_decref(section);
        }
    }

    if (required) {
        for (size_t i = 0; i < required_count; i++) {
            free(required[i]);
        }
        free(required);
    }

    char* dumped = json_dumps(filtered, JSON_COMPACT);
    json_decref(filtered);
    if (!dumped) {
        collector->filter_failed = true;
        snprintf(collector->filter_buffer, sizeof(collector->filter_buffer), "{}");
        return collector->filter_buffer;
    }

    snprintf(collector->filter_buffer, sizeof(collector->filter_buffer), "%s", dumped);
    free(dumped);
    return collector->filter_buffer;
}

/*
 * Filtered parameters JSON for a bare statement (same rule as the
 * transaction-block callback).
 */
char* txn_filtered_parameters(BlockCollector* collector, const char* statement) {
    const char* filtered = txn_parameters_callback(0, statement, collector);
    return filtered ? strdup(filtered) : NULL;
}

/*
 * Build the final QueryResult from the collected entries.
 */
QueryResult* txn_build_result(BlockCollector* collector, bool success,
                              const char* error_message) {
    QueryResult* result = calloc(1, sizeof(QueryResult));
    if (!result) {
        return NULL;
    }

    result->success = success;
    result->row_count = collector->total_rows;
    result->column_count = collector->total_columns;
    result->affected_rows = (int)collector->total_affected;
    result->execution_time_ms = (time_t)collector->total_time_ms;
    result->error_class = success ? DB_ERR_NONE : DB_ERR_OTHER;

    if (collector->entries) {
        result->data_json = json_dumps(collector->entries, JSON_COMPACT);
    }
    if (!result->data_json) {
        result->data_json = strdup("[]");
    }

    if (!success) {
        result->error_message = strdup(error_message ? error_message : "transaction block execution failed");
    }
    return result;
}

/*
 * Execute one bare (non-transaction) statement exactly as the
 * single-statement runtime path does: autocommit, no prepared statement.
 */
bool txn_execute_bare_statement(DatabaseHandle* connection, BlockCollector* collector,
                                const char* statement, QueryRequest* request,
                                QueryResult** statement_result, const char* dqm_label) {
    QueryRequest bare_request = {0};
    bare_request.query_id = request->query_id;
    bare_request.sql_template = (char*)statement;
    bare_request.timeout_seconds = request->timeout_seconds;
    bare_request.isolation_level = request->isolation_level;
    bare_request.use_prepared_statement = false;
    bare_request.prepared_statement_name = NULL;

    char* filtered = txn_filtered_parameters(collector, statement);
    bare_request.parameters_json = filtered ? filtered : request->parameters_json;

    bool executed = database_engine_execute(connection, &bare_request, statement_result);

    if (filtered) {
        free(filtered);
    }

    if (!executed || !*statement_result) {
        log_this(dqm_label, "Bare statement in marked query failed to execute", LOG_LEVEL_ERROR, 0);
        return false;
    }
    return true;
}

bool database_queue_execute_query_with_blocks(DatabaseHandle* connection, QueryRequest* request,
                                              QueryResult** out_result, const char* dqm_label) {
    *out_result = NULL;

    if (!connection || !request || !request->sql_template) {
        QueryResult* result = calloc(1, sizeof(QueryResult));
        if (!result) {
            return false;
        }
        result->success = false;
        result->error_class = DB_ERR_OTHER;
        result->error_message = strdup("invalid transaction block request");
        result->data_json = strdup("[]");
        *out_result = result;
        return false;
    }

    const char* label = dqm_label ? dqm_label : "TXNQ";

    // Parse the markers into blocks.
    TransactionBlockList* blocks = NULL;
    char* parse_error = NULL;
    if (!transaction_blocks_parse(request->sql_template, &blocks, &parse_error)) {
        log_this(label, "Transaction block parse failed: %s", LOG_LEVEL_ERROR, 1,
                 parse_error ? parse_error : "unknown parse error");
        BlockCollector* empty = txn_collector_create(NULL);
        QueryResult* result = empty
            ? txn_build_result(empty, false, parse_error ? parse_error : "invalid transaction block syntax")
            : NULL;
        if (empty) {
            txn_collector_free(empty);
        }
        free(parse_error);
        *out_result = result;
        return false;
    }

    BlockCollector* collector = txn_collector_create(request->parameters_json);
    if (!collector) {
        transaction_blocks_free(blocks);
        *out_result = NULL;
        return false;
    }

    bool success = true;
    char* failure_message = NULL;

    for (size_t b = 0; b < blocks->block_count && success; b++) {
        TransactionBlock* block = &blocks->blocks[b];
        collector->current_block = b + 1;

        if (block->is_transaction) {
            TransactionExecOutcome outcome;
            bool block_ok = database_execute_statements_in_transaction(
                connection, block->statements, block->statement_count,
                NULL, request->timeout_seconds, "runtime_statement", "RTXN",
                true, txn_statement_callback, txn_parameters_callback, collector,
                &outcome, label);

            if (!block_ok) {
                success = false;
                if (outcome.failure_at_begin) {
                    failure_message = strdup("transaction block failed to begin");
                } else if (outcome.failure_at_commit) {
                    failure_message = strdup("transaction block failed to commit");
                } else {
                    failure_message = malloc(192);
                    if (failure_message) {
                        snprintf(failure_message, 192,
                                 "transaction block %zu failed at statement %zu (rolled back)",
                                 b + 1, outcome.failed_statement_index + 1);
                    }
                }
            }
        } else {
            // Bare statement: autocommit, like any unmarked runtime query.
            for (size_t s = 0; s < block->statement_count && success; s++) {
                QueryResult* statement_result = NULL;
                if (!txn_execute_bare_statement(connection, collector, block->statements[s],
                                                request, &statement_result, label)) {
                    if (statement_result) {
                        database_engine_cleanup_result(statement_result);
                    }
                    success = false;
                    failure_message = malloc(192);
                    if (failure_message) {
                        snprintf(failure_message, 192,
                                 "statement %zu outside a transaction block failed", s + 1);
                    }
                    break;
                }

                bool stmt_ok = statement_result->success;
                if (!txn_collect_entry(collector, s, statement_result)) {
                    database_engine_cleanup_result(statement_result);
                    success = false;
                    failure_message = strdup("failed assembling statement results");
                    break;
                }
                if (!stmt_ok) {
                    success = false;
                    failure_message = malloc(256);
                    if (failure_message) {
                        snprintf(failure_message, 256, "statement %zu outside a transaction block failed: %s",
                                 s + 1,
                                 statement_result->error_message ? statement_result->error_message : "engine error");
                    }
                }
                database_engine_cleanup_result(statement_result);
            }
        }
    }

    QueryResult* result = txn_build_result(collector, success, failure_message);
    if (collector->filter_failed && success) {
        // Parameter filtering could not be built for one statement; the
        // engine would have rejected the batch, so surface it as failure.
        if (result) {
            result->success = false;
            result->error_class = DB_ERR_OTHER;
            free(result->error_message);
            result->error_message = strdup("failed filtering parameters for a statement");
        }
        success = false;
    }

    txn_collector_free(collector);
    transaction_blocks_free(blocks);
    free(failure_message);

    *out_result = result;
    if (!result) {
        return false;
    }
    return success;
}
