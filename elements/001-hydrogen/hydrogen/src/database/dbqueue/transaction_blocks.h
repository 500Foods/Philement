/*
 * Transaction Block Parsing for Runtime QueryRefs
 *
 * Runtime QueryRefs may contain explicit multi-statement transaction blocks,
 * authored with line-start markers:
 *
 *     -- TRANSACTION_START
 *     statement one;
 *     -- STATEMENT
 *     statement two;
 *     -- TRANSACTION_END
 *     plain statement outside any block;
 *
 * This is the runtime sibling of the migration system's
 * "-- SUBQUERY DELIMITER" bands. Migrations are NOT affected: they keep
 * their own delimiter and their own executor. The markers are
 * author-placed delimiters (like migration bands), so no semicolon
 * tokenizer or literal parsing is involved here.
 */

#ifndef DATABASE_TRANSACTION_BLOCKS_H
#define DATABASE_TRANSACTION_BLOCKS_H

// Project includes
#include <src/hydrogen.h>

/* Transaction block markers. A marker is only recognized when it is the
 * entire line (leading whitespace allowed), matching the anchoring the
 * migration apply path uses for its delimiter. The text must appear
 * exactly: case-sensitive, nothing else on the line. */
#define TXN_MARK_START     "-- TRANSACTION_START"
#define TXN_MARK_STATEMENT "-- STATEMENT"
#define TXN_MARK_END       "-- TRANSACTION_END"

/*
 * One unit of runtime execution.
 *
 * is_transaction = true: statements run inside a single transaction
 * (begin -> statements -> commit; rollback on any failure).
 * is_transaction = false: a single bare statement outside any block,
 * executed exactly as an unmarked single-statement query is today.
 */
typedef struct TransactionBlock {
    bool is_transaction;
    char** statements;
    size_t statement_count;
} TransactionBlock;

typedef struct TransactionBlockList {
    TransactionBlock* blocks;
    size_t block_count;
} TransactionBlockList;

/*
 * Cheap pre-filter: does the SQL text mention any transaction marker?
 *
 * When this returns false the caller never parses - the existing
 * single-statement runtime path runs, byte for byte unchanged.
 */
bool sql_has_transaction_markers(const char* sql);

/*
 * Parse marker-annotated SQL into an ordered list of blocks.
 *
 * Returns true on success with *out_blocks set (caller frees with
 * transaction_blocks_free). Returns false with *error_message set to a
 * malloc'd description (caller frees) and *out_blocks left NULL.
 *
 * Rejected shapes (each names the offending rule):
 *   - TRANSACTION_END or STATEMENT outside a block
 *   - TRANSACTION_START inside an already-open block
 *   - a block with no executable statements
 *   - TRANSACTION_START without TRANSACTION_END
 */
bool transaction_blocks_parse(const char* sql, TransactionBlockList** out_blocks,
                              char** error_message);

/* Free a list from transaction_blocks_parse. NULL-safe. */
void transaction_blocks_free(TransactionBlockList* list);

/*
 * Internal helpers. Declared here (not static) because the project rule
 * forbids static functions in src/ and the trial build runs with
 * -Wmissing-prototypes; unit tests may call them directly.
 */
char* txn_copy_range(const char* start, size_t length);
char* txn_trim_in_place(char* text);
bool txn_push_statement(char*** statements, size_t* count, char* statement);
bool txn_push_block(TransactionBlockList* list, bool is_transaction,
                    char** statements, size_t count);
bool txn_fail(char** error_message, const char* detail);
int txn_marker_kind(const char* trimmed_line);
bool txn_emit_bare_block(TransactionBlockList* list, char* text, char** error_message);

#endif // DATABASE_TRANSACTION_BLOCKS_H
