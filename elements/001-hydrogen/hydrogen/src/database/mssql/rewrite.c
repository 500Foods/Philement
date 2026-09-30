/*
 * MSSQL Database Engine - RETURNING to OUTPUT Rewrite
 *
 * Bounded rewriter: converts PostgreSQL-style
 *   INSERT INTO table (cols) ... RETURNING col
 * into T-SQL
 *   INSERT INTO table (cols) OUTPUT INSERTED.col ...
 *
 * Only handles the INSERT ... WITH cte ... SELECT ... RETURNING col shape
 * emitted by Helium QueryRefs.  Any shape that cannot be parsed is left
 * untouched (callers treat NULL return as "pass through").
 *
 * Lock 21 in MSSQL.md.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "rewrite.h"

/* Skip whitespace, return pointer to next non-space char */
const char* mssql_skip_ws(const char* s) {
    if (!s) return s;
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

/*
 * Case-insensitive match of `needle` at position `p`.
 * Returns true on match; caller may use *adv to know how far consumed.
 */
bool mssql_match_word(const char* p, const char* needle, size_t* adv) {
    if (!p || !needle) return false;
    size_t nlen = strlen(needle);
    if (strncasecmp(p, needle, nlen) != 0) return false;
    /* Must be followed by whitespace, end-of-string, or ';' */
    char next = p[nlen];
    if (next != '\0' && !isspace((unsigned char)next) && next != ';') return false;
    if (adv) *adv = nlen;
    return true;
}

/*
 * Find the position of "RETURNING" as a keyword (case-insensitive) that is
 * a standalone word.  Returns the offset within `sql` or -1.
 */
long mssql_find_returning(const char* sql) {
    if (!sql) return -1;
    const char* p = sql;
    while (*p) {
        size_t adv = 0;
        if (mssql_match_word(p, "RETURNING", &adv)) {
            /* Check the character immediately before the keyword:
             * it must not be an identifier character (alnum, _, $). */
            if (p > sql) {
                char prev = p[-1];
                if (isalnum((unsigned char)prev) || prev == '_' || prev == '$') {
                    p++;
                    continue;
                }
            }
            return (long)(p - sql);
        }
        p++;
    }
    return -1;
}

/*
 * Extract the column name following RETURNING.
 * Returns a newly allocated string (caller frees) or NULL.
 * Handles: RETURNING col, RETURNING col , RETURNING col;
 */
char* mssql_extract_returning_column(const char* returning_pos) {
    if (!returning_pos) return NULL;

    size_t adv = 0;
    if (!mssql_match_word(returning_pos, "RETURNING", &adv)) return NULL;

    const char* p = returning_pos + adv;
    p = mssql_skip_ws(p);

    /* Collect the column name (identifier chars) */
    const char* start = p;
    while (*p && !isspace((unsigned char)*p) && *p != ';' && *p != ',') {
        p++;
    }
    if (p == start) return NULL;

    size_t len = (size_t)(p - start);
    char* col = calloc(1, len + 1);
    if (!col) return NULL;
    memcpy(col, start, len);
    return col;
}

/*
 * Find the matching closing parenthesis for the INSERT INTO ... (...) column list.
 * `paren_open` points at the opening '('.
 * Returns pointer to the matching ')' or NULL if not found.
 */
const char* mssql_find_insert_col_list_close(const char* sql, long paren_open_offset) {
    if (!sql) return NULL;
    const char* p = sql + paren_open_offset;
    int depth = 0;
    bool in_string = false;
    bool in_comment = false;

    while (*p) {
        if (in_comment) {
            if (*p == '\n') in_comment = false;
            p++;
            continue;
        }
        if (in_string) {
            if (*p == '\'') {
                /* Handle SQL escaped quotes '' */
                if (p[1] == '\'') {
                    p += 2;
                    continue;
                }
                in_string = false;
            }
            p++;
            continue;
        }
        if (*p == '-' && p[1] == '-') {
            in_comment = true;
            p += 2;
            continue;
        }
        if (*p == '\'') {
            in_string = true;
            p++;
            continue;
        }
        if (*p == '(') {
            depth++;
        } else if (*p == ')') {
            depth--;
            if (depth == 0) return p;
        }
        p++;
    }
    return NULL;
}

/*
 * Find the offset of the '(' that opens the INSERT column list.
 * Scans for "INSERT" keyword (case-insensitive) followed by "INTO" then
 * optional whitespace then the table name then '('.
 */
long mssql_find_insert_column_list_open(const char* sql) {
    if (!sql) return -1;
    const char* p = sql;
    while (*p) {
        size_t adv = 0;
        if (mssql_match_word(p, "INSERT", &adv)) {
            p = mssql_skip_ws(p + adv);
            if (mssql_match_word(p, "INTO", &adv)) {
                p = mssql_skip_ws(p + adv);
                /* Skip table name (may be schema-qualified, e.g. testms.accounts) */
                while (*p && !isspace((unsigned char)*p) && *p != '(' && *p != ';') {
                    p++;
                }
                p = mssql_skip_ws(p);
                if (*p == '(') {
                    return (long)(p - sql);
                }
            }
        }
        p++;
    }
    return -1;
}

/*
 * Rewrite INSERT ... RETURNING col  ->  INSERT ... OUTPUT INSERTED.col
 *
 * Returns a newly allocated string (caller frees) or NULL on failure.
 * When NULL is returned the caller should pass the original SQL through.
 */
char* mssql_rewrite_returning_to_output(const char* sql) {
    if (!sql) return NULL;

    long returning_offset = mssql_find_returning(sql);
    if (returning_offset < 0) return NULL;

    char* returning_col = mssql_extract_returning_column(sql + returning_offset);
    if (!returning_col) return NULL;

    /* Trim trailing whitespace and semicolons from returning_col */
    char* end = returning_col + strlen(returning_col) - 1;
    while (end > returning_col && (isspace((unsigned char)*end) || *end == ';' || *end == ',')) {
        *end = '\0';
        end--;
    }
    if (strlen(returning_col) == 0) {
        free(returning_col);
        return NULL;
    }

    long col_list_open = mssql_find_insert_column_list_open(sql);
    if (col_list_open < 0) {
        free(returning_col);
        return NULL;
    }

    const char* close_paren = mssql_find_insert_col_list_close(sql, col_list_open);
    if (!close_paren) {
        free(returning_col);
        return NULL;
    }

    const char* returning_ptr = sql + returning_offset;
    size_t ret_len = strlen(returning_col);

    /*
     * Build result:
     *   [0 .. close_paren] + "\nOUTPUT INSERTED." + returning_col + "\n" +
     *   [close_paren+1 .. returning_ptr]
     *
     * We insert "OUTPUT INSERTED.col" right after the closing ')' of the
     * INSERT column list, then keep the rest of the statement (WITH cte,
     * SELECT, etc.) but strip the trailing RETURNING clause.
     */
    size_t prefix_len = (size_t)(close_paren - sql) + 1;  /* include ')' */
    size_t middle_len = (size_t)(returning_ptr - close_paren) - 1;  /* from after ')' to before RETURNING */

    size_t total = prefix_len + strlen("\nOUTPUT INSERTED.") + ret_len + strlen("\n") + middle_len + 2;
    char* result = calloc(1, total);
    if (!result) {
        free(returning_col);
        return NULL;
    }

    /* Prefix: up to and including ')' */
    memcpy(result, sql, prefix_len);
    size_t pos = prefix_len;

    /* "OUTPUT INSERTED.col" */
    memcpy(result + pos, "\nOUTPUT INSERTED.", strlen("\nOUTPUT INSERTED."));
    pos += strlen("\nOUTPUT INSERTED.");
    memcpy(result + pos, returning_col, ret_len);
    pos += ret_len;

    /* Newline */
    result[pos] = '\n';
    pos++;

    /* Middle: everything from after ')' up to RETURNING */
    if (middle_len > 0) {
        memcpy(result + pos, close_paren + 1, middle_len);
        pos += middle_len;
    }

    /* Ensure no trailing whitespace before end */
    while (pos > 0 && (result[pos-1] == ' ' || result[pos-1] == '\t' || result[pos-1] == '\n' || result[pos-1] == '\r')) {
        pos--;
    }
    result[pos] = '\0';

    free(returning_col);
    return result;
}

/*
 * Helper: find the opening '(' of a CTE definition, i.e., the '(' right after
 * "WITH name AS".  Returns pointer into `sql` or NULL.
 */
const char* mssql_find_cte_open_paren(const char* sql) {
    if (!sql) return NULL;
    const char* p = sql;
    while (*p) {
        size_t adv = 0;
        bool boundary = (p == sql) ||
            !(isalnum((unsigned char)p[-1]) || p[-1] == '_' || p[-1] == '$');
        if (boundary && mssql_match_word(p, "WITH", &adv)) {
            const char* q = mssql_skip_ws(p + adv);
            const char* name = q;
            /* CTE name, then an optional column list: WITH digits(i) AS ( */
            while (*q && (isalnum((unsigned char)*q) || *q == '_' || *q == '.')) q++;
            if (q != name) {
                q = mssql_skip_ws(q);
                if (*q == '(') {
                    const char* cols_close = mssql_find_insert_col_list_close(sql, (long)(q - sql));
                    if (cols_close) q = mssql_skip_ws(cols_close + 1);
                }
                if (mssql_match_word(q, "AS", &adv)) {
                    q = mssql_skip_ws(q + adv);
                    if (*q == '(') return q;
                }
            }
        }
        p++;
    }
    return NULL;
}

/*
 * Rewrite `INSERT INTO table (cols) WITH cte AS (...) SELECT ...`
 * into `WITH cte AS (...) INSERT INTO table (cols) SELECT ...`.
 */
char* mssql_rewrite_insert_with_to_with_insert(const char* sql) {
    if (!sql) return NULL;

    /* Look for INSERT keyword */
    long insert_offset = -1;
    const char* p = sql;
    while (*p) {
        size_t adv = 0;
        if (mssql_match_word(p, "INSERT", &adv)) {
            const char* after_insert = p + adv;
            after_insert = mssql_skip_ws(after_insert);
            if (mssql_match_word(after_insert, "INTO", &adv)) {
                insert_offset = (long)(p - sql);
                break;
            }
        }
        p++;
    }
    if (insert_offset < 0) return NULL;

    /* Find the WITH keyword that follows the INSERT...INTO...table...(cols) */
    size_t adv = 0;
    const char* after_into = p + strlen("INSERT");
    after_into = mssql_skip_ws(after_into);
    if (!mssql_match_word(after_into, "INTO", &adv)) return NULL;
    const char* past_into = mssql_skip_ws(after_into + adv);
    /* Skip table name */
    while (*past_into && !isspace((unsigned char)*past_into) && *past_into != '(' && *past_into != ';') {
        past_into++;
    }
    past_into = mssql_skip_ws(past_into);

    /* If there's a column list, skip past its closing ')' */
    if (*past_into == '(') {
        const char* close_paren = mssql_find_insert_col_list_close(sql, (long)(past_into - sql));
        if (!close_paren) return NULL;
        past_into = mssql_skip_ws(close_paren + 1);
    }

    /* Now past_into should point at "WITH" */
    if (!mssql_match_word(past_into, "WITH", &adv)) {
        return NULL;
    }

    const char* with_ptr = past_into;
    long with_offset = (long)(with_ptr - sql);

    /* Find matching ')' for the CTE definition */
    const char* cte_open = mssql_find_cte_open_paren(sql);
    if (!cte_open) return NULL;
    const char* cte_close = mssql_find_insert_col_list_close(sql, (long)(cte_open - sql));
    if (!cte_close) return NULL;

    /* Build: WITH ... cte ... INSERT INTO ... (cols) SELECT ... */
    /* = [with_offset .. cte_close] + [insert_offset .. with_offset] + [cte_close+1 .. end] */
    size_t cte_len = (size_t)(cte_close - with_ptr) + 1;  /* WITH ... cte ... ) */
    size_t insert_len = (size_t)(with_offset - insert_offset);  /* INSERT INTO ... */
    size_t rest_start = (size_t)(cte_close - sql) + 1;
    size_t rest_len = strlen(sql + rest_start);

    size_t total = cte_len + insert_len + rest_len + 3;
    char* result = calloc(1, total);
    if (!result) return NULL;

    size_t pos = 0;
    memcpy(result + pos, with_ptr, cte_len);
    pos += cte_len;
    result[pos] = '\n';
    pos++;
    memcpy(result + pos, sql + insert_offset, insert_len);
    pos += insert_len;
    memcpy(result + pos, sql + rest_start, rest_len);
    pos += rest_len;
    result[pos] = '\0';

    return result;
}

/*
 * PostgreSQL accepts WITH name(cols) AS (VALUES (...),(...)).
 * SQL Server requires a SELECT. Wrap the list as
 * SELECT * FROM (VALUES ...) AS v(cols). One CTE per call.
 * Returns NULL when this shape is absent.
 */
char* mssql_rewrite_cte_values(const char* sql) {
    if (!sql || !*sql) return NULL;

    const char* p = sql;
    const char* cte_open = NULL;
    const char* cte_close = NULL;
    const char* values_at = NULL;
    const char* cols = NULL;
    size_t cols_len = 0;

    while (*p && !cte_open) {
        if (*p == '\'') {
            p++;
            while (*p) {
                if (*p == '\'' && p[1] == '\'') {
                    p += 2;
                    continue;
                }
                if (*p == '\'') {
                    p++;
                    break;
                }
                p++;
            }
            continue;
        }
        if (*p == '-' && p[1] == '-') {
            while (*p && *p != '\n') p++;
            continue;
        }

        size_t adv = 0;
        bool boundary = (p == sql) ||
            !(isalnum((unsigned char)p[-1]) || p[-1] == '_' || p[-1] == '$');
        if (boundary && mssql_match_word(p, "WITH", &adv)) {
            const char* q = mssql_skip_ws(p + adv);
            const char* name = q;
            while (*q && (isalnum((unsigned char)*q) || *q == '_' || *q == '.')) q++;
            if (q != name) {
                q = mssql_skip_ws(q);
                const char* col_s = NULL;
                const char* col_e = NULL;
                if (*q == '(') {
                    const char* col_close = mssql_find_insert_col_list_close(sql, (long)(q - sql));
                    if (col_close) {
                        col_s = q + 1;
                        col_e = col_close;
                        q = mssql_skip_ws(col_close + 1);
                    }
                }
                if (col_s && mssql_match_word(q, "AS", &adv)) {
                    q = mssql_skip_ws(q + adv);
                    if (*q == '(') {
                        const char* open = q;
                        const char* close = mssql_find_insert_col_list_close(sql, (long)(open - sql));
                        if (close) {
                            const char* body = mssql_skip_ws(open + 1);
                            if (mssql_match_word(body, "VALUES", &adv)) {
                                while (col_s < col_e && isspace((unsigned char)*col_s)) col_s++;
                                while (col_e > col_s && isspace((unsigned char)col_e[-1])) col_e--;
                                if (col_s < col_e) {
                                    cte_open = open;
                                    cte_close = close;
                                    values_at = body;
                                    cols = col_s;
                                    cols_len = (size_t)(col_e - col_s);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
        p++;
    }

    if (!cte_open || !cte_close || !values_at || !cols || cols_len == 0) return NULL;

    const char* prefix_wrap = "SELECT * FROM (";
    const char* mid_wrap = ") AS v(";
    const char* end_wrap = ")";
    size_t prefix_len = strlen(prefix_wrap);
    size_t mid_len = strlen(mid_wrap);
    size_t end_len = strlen(end_wrap);
    size_t values_len = (size_t)(cte_close - values_at);
    while (values_len > 0 && isspace((unsigned char)values_at[values_len - 1])) values_len--;

    size_t head_len = (size_t)(cte_open - sql) + 1;
    size_t tail_len = strlen(cte_close);
    size_t total = head_len + prefix_len + values_len + mid_len + cols_len + end_len + tail_len + 1;
    char* result = calloc(1, total);
    if (!result) return NULL;

    size_t pos = 0;
    memcpy(result + pos, sql, head_len);
    pos += head_len;
    memcpy(result + pos, prefix_wrap, prefix_len);
    pos += prefix_len;
    memcpy(result + pos, values_at, values_len);
    pos += values_len;
    memcpy(result + pos, mid_wrap, mid_len);
    pos += mid_len;
    memcpy(result + pos, cols, cols_len);
    pos += cols_len;
    memcpy(result + pos, end_wrap, end_len);
    pos += end_len;
    memcpy(result + pos, cte_close, tail_len);
    pos += tail_len;
    result[pos] = '\0';
    return result;
}

/*
 * PostgreSQL ALTER TABLE ... ADD COLUMN col becomes T-SQL ADD col.
 * DROP COLUMN is already valid T-SQL and is left unchanged.
 * Returns NULL when no ADD COLUMN keyword is present outside strings.
 */
char* mssql_rewrite_add_column(const char* sql) {
    if (!sql || !*sql) return NULL;

    size_t cap = strlen(sql) + 1;
    char* out = malloc(cap);
    if (!out) return NULL;

    size_t n = 0;
    bool changed = false;
    const char* p = sql;

    while (*p) {
        if (*p == '\'') {
            out[n++] = *p++;
            while (*p) {
                if (*p == '\'' && p[1] == '\'') {
                    out[n++] = *p++;
                    out[n++] = *p++;
                    continue;
                }
                out[n++] = *p;
                if (*p == '\'') {
                    p++;
                    break;
                }
                p++;
            }
            continue;
        }
        if (*p == '-' && p[1] == '-') {
            while (*p && *p != '\n') out[n++] = *p++;
            continue;
        }

        size_t adv = 0;
        bool boundary = (p == sql) ||
            !(isalnum((unsigned char)p[-1]) || p[-1] == '_' || p[-1] == '$');
        if (boundary && mssql_match_word(p, "ADD", &adv)) {
            const char* after_add = mssql_skip_ws(p + adv);
            size_t col_adv = 0;
            if (mssql_match_word(after_add, "COLUMN", &col_adv)) {
                size_t keep = (size_t)(after_add - p);
                memcpy(out + n, p, keep);
                n += keep;
                p = mssql_skip_ws(after_add + col_adv);
                changed = true;
                continue;
            }
        }
        out[n++] = *p++;
    }

    if (!changed) {
        free(out);
        return NULL;
    }
    out[n] = '\0';
    return out;
}

/*
 * Apply the MSSQL rewrites used by both direct execution and prepare:
 * RETURNING, then a bare VALUES CTE body, then INSERT...WITH order,
 * then ADD COLUMN → ADD. Returns NULL when sql needs none of them.
 */
char* mssql_rewrite_migration_sql(const char* sql) {
    if (!sql) return NULL;

    char* current = NULL;
    const char* base = sql;

    char* returning = mssql_rewrite_returning_to_output(sql);
    if (returning) {
        current = returning;
        base = current;
    } else {
        char* values = NULL;
        for (int pass = 0; pass < 4; pass++) {
            char* next = mssql_rewrite_cte_values(base);
            if (!next) break;
            free(values);
            values = next;
            base = values;
        }

        char* moved = mssql_rewrite_insert_with_to_with_insert(base);
        if (moved) {
            free(values);
            current = moved;
            base = current;
        } else if (values) {
            current = values;
            base = current;
        }
    }

    char* added = mssql_rewrite_add_column(base);
    if (added) {
        free(current);
        return added;
    }
    return current;
}
