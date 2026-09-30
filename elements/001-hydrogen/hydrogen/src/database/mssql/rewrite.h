/*
 * MSSQL Database Engine - SQL Rewrite Header
 *
 * Bounded rewriters for locks 21-22 in MSSQL.md:
 * - Lock 21: Convert PostgreSQL-style RETURNING to T-SQL OUTPUT
 * - Lock 22: Rewrite INSERT INTO ... WITH cte ... SELECT to WITH cte ... INSERT INTO ... SELECT
 * - Bare CTE body VALUES (...) becomes SELECT * FROM (VALUES ...) AS v(cols)
 * - ALTER TABLE ... ADD COLUMN col becomes ADD col (DROP COLUMN stays)
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

/* Find the opening '(' of a CTE body (after "WITH name[(cols)] AS") */
const char* mssql_find_cte_open_paren(const char* sql);

/*
 * Rewrite INSERT INTO ... RETURNING col  ->  INSERT INTO ... OUTPUT INSERTED.col
 * Returns newly allocated string (caller frees) or NULL.
 */
char* mssql_rewrite_returning_to_output(const char* sql);

/*
 * Rewrite `INSERT INTO table (cols) WITH cte AS (...) SELECT ... FROM cte`
 * into T-SQL-compatible `WITH cte AS (...) INSERT INTO table (cols) SELECT ... FROM cte`.
 *
 * Lock 22 in MSSQL.md. Returns newly allocated string (caller frees) or NULL
 * if the shape does not match (caller passes original SQL through).
 */
char* mssql_rewrite_insert_with_to_with_insert(const char* sql);

/*
 * Rewrite the first WITH name(cols) AS (VALUES ...) body to
 * WITH name(cols) AS (SELECT * FROM (VALUES ...) AS v(cols)).
 * Returns a new string (caller frees) or NULL when the shape is absent.
 */
char* mssql_rewrite_cte_values(const char* sql);

/*
 * Rewrite ALTER TABLE ... ADD COLUMN col to ADD col.
 * DROP COLUMN is unchanged. Returns a new string (caller frees)
 * or NULL when the keyword is absent.
 */
char* mssql_rewrite_add_column(const char* sql);

/*
 * RETURNING, then bare VALUES CTE bodies, then INSERT...WITH order,
 * then ADD COLUMN. Returns a new string (caller frees) or NULL
 * when sql is unchanged.
 */
char* mssql_rewrite_migration_sql(const char* sql);

#endif /* DATABASE_ENGINE_MSSQL_REWRITE_H */
