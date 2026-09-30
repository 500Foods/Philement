/*
 * MSSQL Database Engine - RETURNING to OUTPUT Rewrite Header
 *
 * Bounded rewriter for lock 21 in MSSQL.md: converts
 *   INSERT INTO table (cols) ... RETURNING col
 * to
 *   INSERT INTO table (cols) OUTPUT INSERTED.col ...
 *
 * Fail-closed: returns NULL when the shape cannot be parsed, so callers
 * pass the original SQL through untouched.
 */

#ifndef DATABASE_ENGINE_MSSQL_REWRITE_H
#define DATABASE_ENGINE_MSSQL_REWRITE_H

#include <stdbool.h>

/* Skip whitespace; returns pointer to first non-space char */
const char* mssql_skip_ws(const char* s);

/* Case-insensitive keyword match; *adv gets consumed length */
bool mssql_match_word(const char* p, const char* needle, size_t* adv);

/* Find offset of RETURNING keyword (standalone word), or -1 */
long mssql_find_returning(const char* sql);

/* Extract column name following RETURNING; caller frees */
char* mssql_extract_returning_column(const char* returning_pos);

/* Find matching ')' for the INSERT column-list '(' at given offset */
const char* mssql_find_insert_col_list_close(const char* sql, long paren_open_offset);

/* Find offset of the '(' opening INSERT column list, or -1 */
long mssql_find_insert_column_list_open(const char* sql);

/*
 * Main rewrite entry point.
 * Returns newly allocated rewritten string (caller frees) or NULL.
 * When NULL, caller should use the original SQL unmodified.
 */
char* mssql_rewrite_returning_to_output(const char* sql);

#endif /* DATABASE_ENGINE_MSSQL_REWRITE_H */
