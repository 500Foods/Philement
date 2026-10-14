/*
 * Transaction Block Parsing for Runtime QueryRefs
 *
 * See transaction_blocks.h for the marker syntax and the parser contract.
 * Reuses statement_has_executable_sql from the migration module so
 * comment-only and empty segments are skipped the same way the migration
 * path skips them.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/migration/migration.h>

// Local includes
#include "transaction_blocks.h"

/*
 * Copy a range of the SQL text into a fresh NUL-terminated string.
 * A zero-length range yields an allocated empty string. Returns NULL only
 * on a NULL start or allocation failure.
 */
char* txn_copy_range(const char* start, size_t length) {
    if (!start) {
        return NULL;
    }

    char* copy = malloc(length + 1);
    if (!copy) {
        return NULL;
    }

    if (length > 0) {
        memcpy(copy, start, length);
    }
    copy[length] = '\0';
    return copy;
}

/*
 * Trim leading and trailing whitespace in place.
 * Returns a pointer inside the original string.
 */
char* txn_trim_in_place(char* text) {
    if (!text) {
        return NULL;
    }

    while (*text != '\0' && isspace((unsigned char)*text)) {
        text++;
    }

    if (*text == '\0') {
        return text;
    }

    char* end = text + strlen(text) - 1;
    while (end > text && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }

    return text;
}

/*
 * Append a statement to a dynamic statement array. Takes ownership of the
 * string on success; frees it and returns false on failure.
 */
bool txn_push_statement(char*** statements, size_t* count, char* statement) {
    if (!statements || !count || !statement) {
        free(statement);
        return false;
    }

    char** grown = realloc(*statements, (*count + 1) * sizeof(char*));
    if (!grown) {
        free(statement);
        return false;
    }

    *statements = grown;
    (*statements)[*count] = statement;
    (*count)++;
    return true;
}

/*
 * Append a finished block to the list. Takes ownership of the statement
 * array on success; frees the statements on failure.
 */
bool txn_push_block(TransactionBlockList* list, bool is_transaction,
                    char** statements, size_t count) {
    if (!list) {
        for (size_t i = 0; i < count; i++) {
            free(statements[i]);
        }
        free(statements);
        return false;
    }

    TransactionBlock* grown = realloc(list->blocks, (list->block_count + 1) * sizeof(TransactionBlock));
    if (!grown) {
        for (size_t i = 0; i < count; i++) {
            free(statements[i]);
        }
        free(statements);
        return false;
    }

    list->blocks = grown;
    list->blocks[list->block_count].is_transaction = is_transaction;
    list->blocks[list->block_count].statements = statements;
    list->blocks[list->block_count].statement_count = count;
    list->block_count++;
    return true;
}

/*
 * Record a parse failure with a message built in the caller's buffer.
 * Always returns false so callers can `return txn_fail(...);`.
 */
bool txn_fail(char** error_message, const char* detail) {
    if (error_message) {
        free(*error_message);
        *error_message = strdup(detail ? detail : "invalid transaction block syntax");
    }
    return false;
}

bool sql_has_transaction_markers(const char* sql) {
    if (!sql) {
        return false;
    }

    return strstr(sql, TXN_MARK_START) != NULL ||
           strstr(sql, TXN_MARK_STATEMENT) != NULL ||
           strstr(sql, TXN_MARK_END) != NULL;
}

/*
 * Check whether a trimmed line is exactly one of the markers.
 * Returns 1 for START, 2 for STATEMENT, 3 for END, 0 for none.
 */
int txn_marker_kind(const char* trimmed_line) {
    if (!trimmed_line) {
        return 0;
    }
    if (strcmp(trimmed_line, TXN_MARK_START) == 0) {
        return 1;
    }
    if (strcmp(trimmed_line, TXN_MARK_STATEMENT) == 0) {
        return 2;
    }
    if (strcmp(trimmed_line, TXN_MARK_END) == 0) {
        return 3;
    }
    return 0;
}

/*
 * Emit a bare (non-transaction) statement region as a one-statement block.
 * Takes ownership of text. Regions with no executable content are skipped.
 * Returns false with error_message set on allocation failure.
 */
bool txn_emit_bare_block(TransactionBlockList* list, char* text, char** error_message) {
    if (!text) {
        return true;
    }

    char* trimmed = txn_trim_in_place(text);
    if (!trimmed || *trimmed == '\0' || !statement_has_executable_sql(trimmed)) {
        free(text);
        return true;
    }

    char** one = malloc(sizeof(char*));
    if (!one) {
        free(text);
        return txn_fail(error_message, "out of memory storing bare statement");
    }

    one[0] = strdup(trimmed);
    free(text);
    if (!one[0]) {
        free(one);
        return txn_fail(error_message, "out of memory copying bare statement");
    }

    if (!txn_push_block(list, false, one, 1)) {
        return txn_fail(error_message, "out of memory storing bare statement block");
    }

    return true;
}

bool transaction_blocks_parse(const char* sql, TransactionBlockList** out_blocks,
                              char** error_message) {
    if (error_message) {
        *error_message = NULL;
    }
    if (out_blocks) {
        *out_blocks = NULL;
    }

    if (!sql || !out_blocks) {
        return txn_fail(error_message, "no SQL text to parse");
    }

    TransactionBlockList* list = calloc(1, sizeof(TransactionBlockList));
    if (!list) {
        return txn_fail(error_message, "out of memory parsing transaction blocks");
    }

    // Statement text accumulated for the region currently being read.
    char* current = NULL;
    size_t current_len = 0;
    size_t current_cap = 0;

    // Statements of the transaction block currently open (NULL outside one).
    char** block_statements = NULL;
    size_t block_statement_count = 0;

    bool in_block = false;
    size_t block_start_line = 0;
    size_t line_number = 0;
    bool failed = false;
    char detail[256];

    const char* cursor = sql;
    while (*cursor != '\0' && !failed) {
        const char* eol = strchr(cursor, '\n');
        size_t line_len = eol ? (size_t)(eol - cursor) : strlen(cursor);
        line_number++;

        // Work on a NUL-terminated copy so trimming cannot touch the input.
        char* raw = txn_copy_range(cursor, line_len);
        if (!raw) {
            txn_fail(error_message, "out of memory reading transaction block line");
            failed = true;
            break;
        }

        char* trimmed = txn_trim_in_place(raw);
        int marker = txn_marker_kind(trimmed);

        if (marker == 1) {
            // TRANSACTION_START: flush any pending bare statement first.
            if (in_block) {
                snprintf(detail, sizeof(detail),
                         "TRANSACTION_START inside an open transaction block (line %zu)",
                         line_number);
                free(raw);
                txn_fail(error_message, detail);
                failed = true;
                break;
            }

            if (current_len > 0) {
                if (!txn_emit_bare_block(list, current, error_message)) {
                    free(raw);
                    failed = true;
                    break;
                }
                current = NULL;
                current_len = 0;
                current_cap = 0;
            }

            in_block = true;
            block_start_line = line_number;
        } else if (marker == 2 || marker == 3) {
            // STATEMENT (2) closes one statement; END (3) closes the block.
            if (!in_block) {
                snprintf(detail, sizeof(detail),
                         "%s outside a transaction block (line %zu)",
                         marker == 2 ? "STATEMENT" : "TRANSACTION_END",
                         line_number);
                free(raw);
                txn_fail(error_message, detail);
                failed = true;
                break;
            }

            // Close the statement accumulated so far into the block.
            if (current_len > 0) {
                char* stmt = txn_trim_in_place(current);
                if (stmt && *stmt != '\0' && statement_has_executable_sql(stmt)) {
                    char* owned = strdup(stmt);
                    if (!owned ||
                        !txn_push_statement(&block_statements, &block_statement_count, owned)) {
                        free(raw);
                        txn_fail(error_message, "out of memory storing transaction statement");
                        failed = true;
                        break;
                    }
                }
                free(current);
                current = NULL;
                current_len = 0;
                current_cap = 0;
            }

            if (marker == 3) {
                if (block_statement_count == 0) {
                    snprintf(detail, sizeof(detail),
                             "empty transaction block: TRANSACTION_START at line %zu has no statements",
                             block_start_line);
                    free(raw);
                    txn_fail(error_message, detail);
                    failed = true;
                    break;
                }

                if (!txn_push_block(list, true, block_statements, block_statement_count)) {
                    free(raw);
                    txn_fail(error_message, "out of memory storing transaction block");
                    failed = true;
                    break;
                }

                block_statements = NULL;
                block_statement_count = 0;
                in_block = false;
            }
        } else {
            // Ordinary SQL line: append verbatim (newline included) to the
            // region being accumulated.
            size_t needed = current_len + line_len + (eol ? 1 : 0) + 1;
            if (needed > current_cap) {
                size_t new_cap = current_cap ? current_cap : 128;
                while (new_cap < needed) {
                    new_cap *= 2;
                }
                char* grown = realloc(current, new_cap);
                if (!grown) {
                    free(raw);
                    txn_fail(error_message, "out of memory accumulating statement text");
                    failed = true;
                    break;
                }
                current = grown;
                current_cap = new_cap;
            }
            if (line_len > 0) {
                memcpy(current + current_len, cursor, line_len);
                current_len += line_len;
            }
            if (eol) {
                current[current_len++] = '\n';
            }
            current[current_len] = '\0';
        }

        free(raw);
        cursor = eol ? eol + 1 : cursor + line_len;
    }

    if (!failed && in_block) {
        snprintf(detail, sizeof(detail),
                 "unterminated transaction block: TRANSACTION_START at line %zu has no TRANSACTION_END",
                 block_start_line);
        txn_fail(error_message, detail);
        failed = true;
    }

    if (!failed && current_len > 0) {
        // Trailing bare statement after the last block. The emit helper
        // takes ownership of the text, so drop our pointer afterwards.
        if (!txn_emit_bare_block(list, current, error_message)) {
            failed = true;
        }
        current = NULL;
    }

    free(current);

    if (!failed) {
        for (size_t i = 0; i < block_statement_count; i++) {
            free(block_statements[i]);
        }
        free(block_statements);
        *out_blocks = list;
        return true;
    }

    // Failure: release any dangling block statements and the partial list.
    for (size_t i = 0; i < block_statement_count; i++) {
        free(block_statements[i]);
    }
    free(block_statements);
    transaction_blocks_free(list);
    return false;
}

void transaction_blocks_free(TransactionBlockList* list) {
    if (!list) {
        return;
    }

    for (size_t i = 0; i < list->block_count; i++) {
        TransactionBlock* block = &list->blocks[i];
        for (size_t j = 0; j < block->statement_count; j++) {
            free(block->statements[j]);
        }
        free(block->statements);
    }
    free(list->blocks);
    free(list);
}
