/*
 * Hand-rolled parser for the Acuranzo SQL subset the Firebase engine
 * must run. Unknown verbs become UNSUPPORTED (later phases) or a
 * parse error.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include "sql_parse.h"

#include <ctype.h>

static const char* const FIREBASE_KNOWN_FUNCTIONS[] = {
    "FB_BASE64_DECODE",
    "FB_BASE64_ENCODE",
    "FB_BROTLI_DECOMPRESS",
    "FB_SHA256_B64",
    "FB_JSON_INGEST",
    "FB_JSON_VALUE",
    "FB_NOW",
    "FB_CONVERT_TZ",
    "FB_REFUSE_DROP",
    "FB_TIME_ADD",
    "FB_SESSION_SECS"
};

void firebase_sql_tolower_inplace(char* text) {
    if (!text) {
        return;
    }
    for (size_t i = 0; text[i]; i++) {
        text[i] = (char)tolower((unsigned char)text[i]);
    }
}

void firebase_sql_skip(const char** cursor) {
    if (!cursor || !*cursor) {
        return;
    }
    const char* p = *cursor;
    while (*p) {
        if (isspace((unsigned char)*p)) {
            p++;
            continue;
        }
        if (p[0] == '-' && p[1] == '-') {
            p += 2;
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
    *cursor = p;
}

bool firebase_sql_match_keyword(const char** cursor, const char* keyword) {
    if (!cursor || !*cursor || !keyword) {
        return false;
    }
    firebase_sql_skip(cursor);
    const char* p = *cursor;
    size_t n = strlen(keyword);
    if (strncasecmp(p, keyword, n) != 0) {
        return false;
    }
    unsigned char after = (unsigned char)p[n];
    if (isalnum(after) || after == '_') {
        return false;
    }
    *cursor = p + n;
    return true;
}

bool firebase_sql_parse_ident(const char** cursor, char** out) {
    if (!cursor || !*cursor || !out) {
        return false;
    }
    firebase_sql_skip(cursor);
    const char* p = *cursor;
    if (!isalpha((unsigned char)*p) && *p != '_') {
        return false;
    }
    const char* start = p;
    p++;
    while (isalnum((unsigned char)*p) || *p == '_') {
        p++;
    }
    size_t n = (size_t)(p - start);
    char* ident = malloc(n + 1);
    if (!ident) {
        return false;
    }
    memcpy(ident, start, n);
    ident[n] = '\0';
    *cursor = p;
    *out = ident;
    return true;
}

bool firebase_sql_parse_string(const char** cursor, char** out) {
    if (!cursor || !*cursor || !out) {
        return false;
    }
    firebase_sql_skip(cursor);
    const char* p = *cursor;
    if (*p != '\'') {
        return false;
    }
    p++;
    size_t cap = 32;
    size_t n = 0;
    char* buf = malloc(cap);
    if (!buf) {
        return false;
    }
    while (*p) {
        if (*p == '\'') {
            if (p[1] == '\'') {
                if (n + 1 >= cap) {
                    cap *= 2;
                    char* grown = realloc(buf, cap);
                    if (!grown) {
                        free(buf);
                        return false;
                    }
                    buf = grown;
                }
                buf[n++] = '\'';
                p += 2;
                continue;
            }
            p++;
            buf[n] = '\0';
            *cursor = p;
            *out = buf;
            return true;
        }
        if (n + 1 >= cap) {
            cap *= 2;
            char* grown = realloc(buf, cap);
            if (!grown) {
                free(buf);
                return false;
            }
            buf = grown;
        }
        buf[n++] = *p++;
    }
    free(buf);
    return false;
}

char* firebase_sql_parse_literal(const char** cursor) {
    if (!cursor || !*cursor) {
        return NULL;
    }
    firebase_sql_skip(cursor);
    if (**cursor == '\'') {
        char* inner = NULL;
        if (!firebase_sql_parse_string(cursor, &inner) || !inner) {
            return NULL;
        }
        size_t n = strlen(inner);
        char* quoted = malloc(n + 3);
        if (!quoted) {
            free(inner);
            return NULL;
        }
        quoted[0] = '\'';
        memcpy(quoted + 1, inner, n);
        quoted[n + 1] = '\'';
        quoted[n + 2] = '\0';
        free(inner);
        return quoted;
    }
    const char* p = *cursor;
    if (*p == '+' || *p == '-') {
        p++;
    }
    if (!isdigit((unsigned char)*p)) {
        return NULL;
    }
    while (isdigit((unsigned char)*p)) {
        p++;
    }
    if (*p == '.') {
        p++;
        while (isdigit((unsigned char)*p)) {
            p++;
        }
    }
    size_t n = (size_t)(p - *cursor);
    char* out = malloc(n + 1);
    if (!out) {
        return NULL;
    }
    memcpy(out, *cursor, n);
    out[n] = '\0';
    *cursor = p;
    return out;
}

bool firebase_sql_type_is_known(const char* type) {
    if (!type) {
        return false;
    }
    return strcasecmp(type, "integer") == 0 ||
           strcasecmp(type, "bigint") == 0 ||
           strcasecmp(type, "real") == 0 ||
           strcasecmp(type, "text") == 0 ||
           strcasecmp(type, "json") == 0 ||
           strcasecmp(type, "timestamp") == 0;
}

bool firebase_sql_is_known_function(const char* name) {
    if (!name) {
        return false;
    }
    for (size_t i = 0; i < sizeof(FIREBASE_KNOWN_FUNCTIONS) / sizeof(FIREBASE_KNOWN_FUNCTIONS[0]); i++) {
        if (strcasecmp(name, FIREBASE_KNOWN_FUNCTIONS[i]) == 0) {
            return true;
        }
    }
    return false;
}

void firebase_sql_key_free(FirebaseSqlKey* key) {
    if (!key) {
        return;
    }
    if (key->columns) {
        for (size_t i = 0; i < key->count; i++) {
            free(key->columns[i]);
        }
        free(key->columns);
    }
    key->columns = NULL;
    key->count = 0;
}

bool firebase_sql_key_add(FirebaseSqlKey* key, char* ident) {
    if (!key || !ident) {
        free(ident);
        return false;
    }
    char** grown = realloc(key->columns, (key->count + 1) * sizeof(char*));
    if (!grown) {
        free(ident);
        return false;
    }
    key->columns = grown;
    key->columns[key->count] = ident;
    key->count++;
    return true;
}

bool firebase_sql_parse_ident_list(const char** cursor, FirebaseSqlKey* key) {
    if (!cursor || !*cursor || !key) {
        return false;
    }
    firebase_sql_skip(cursor);
    if (**cursor != '(') {
        return false;
    }
    (*cursor)++;
    while (1) {
        char* ident = NULL;
        if (!firebase_sql_parse_ident(cursor, &ident)) {
            return false;
        }
        if (!firebase_sql_key_add(key, ident)) {
            return false;
        }
        firebase_sql_skip(cursor);
        if (**cursor == ',') {
            (*cursor)++;
            continue;
        }
        if (**cursor == ')') {
            (*cursor)++;
            return key->count > 0;
        }
        return false;
    }
}

void firebase_sql_column_free(FirebaseSqlColumn* col) {
    if (!col) {
        return;
    }
    free(col->name);
    free(col->type);
    free(col->default_sql);
    col->name = NULL;
    col->type = NULL;
    col->default_sql = NULL;
    col->not_null = false;
}

void firebase_sql_table_free(FirebaseSqlTable* table) {
    if (!table) {
        return;
    }
    free(table->name);
    if (table->columns) {
        for (size_t i = 0; i < table->column_count; i++) {
            firebase_sql_column_free(&table->columns[i]);
        }
        free(table->columns);
    }
    firebase_sql_key_free(&table->primary_key);
    if (table->unique_keys) {
        for (size_t i = 0; i < table->unique_count; i++) {
            firebase_sql_key_free(&table->unique_keys[i]);
        }
        free(table->unique_keys);
    }
    memset(table, 0, sizeof(*table));
}

FirebaseSqlStatement* firebase_sql_statement_alloc(void) {
    return calloc(1, sizeof(FirebaseSqlStatement));
}

void firebase_sql_where_free(FirebaseSqlWhere* where) {
    if (!where) {
        return;
    }
    if (where->predicates) {
        for (size_t i = 0; i < where->count; i++) {
            free(where->predicates[i].column);
            firebase_expr_free(where->predicates[i].value);
            if (where->predicates[i].in_values) {
                for (size_t j = 0; j < where->predicates[i].in_count; j++) {
                    firebase_expr_free(where->predicates[i].in_values[j]);
                }
                free(where->predicates[i].in_values);
            }
        }
        free(where->predicates);
    }
    memset(where, 0, sizeof(*where));
}

void firebase_sql_insert_free(FirebaseSqlInsert* insert) {
    if (!insert) {
        return;
    }
    free(insert->table);
    if (insert->columns) {
        for (size_t i = 0; i < insert->column_count; i++) {
            free(insert->columns[i]);
        }
        free(insert->columns);
    }
    if (insert->rows) {
        for (size_t r = 0; r < insert->row_count; r++) {
            if (insert->rows[r].values) {
                for (size_t c = 0; c < insert->rows[r].count; c++) {
                    firebase_expr_free(insert->rows[r].values[c]);
                }
                free(insert->rows[r].values);
            }
        }
        free(insert->rows);
    }
    memset(insert, 0, sizeof(*insert));
}

void firebase_sql_update_free(FirebaseSqlUpdate* update) {
    if (!update) {
        return;
    }
    free(update->table);
    if (update->sets) {
        for (size_t i = 0; i < update->set_count; i++) {
            free(update->sets[i].column);
            firebase_expr_free(update->sets[i].value);
        }
        free(update->sets);
    }
    firebase_sql_where_free(&update->where);
    memset(update, 0, sizeof(*update));
}

void firebase_sql_delete_free(FirebaseSqlDelete* del) {
    if (!del) {
        return;
    }
    free(del->table);
    firebase_sql_where_free(&del->where);
    memset(del, 0, sizeof(*del));
}

bool firebase_sql_kind_is_dml(FirebaseSqlKind kind) {
    return kind == FIREBASE_SQL_KIND_INSERT ||
           kind == FIREBASE_SQL_KIND_UPDATE ||
           kind == FIREBASE_SQL_KIND_DELETE;
}

void firebase_sql_statement_free(FirebaseSqlStatement* stmt) {
    if (!stmt) {
        return;
    }
    free(stmt->error_message);
    firebase_sql_table_free(&stmt->table);
    free(stmt->index.name);
    free(stmt->index.table);
    firebase_sql_key_free(&stmt->index.columns);
    free(stmt->alter.table);
    firebase_sql_column_free(&stmt->alter.add_column);
    free(stmt->alter.drop_column);
    free(stmt->alter.rename_to);
    firebase_sql_insert_free(&stmt->insert);
    firebase_sql_update_free(&stmt->update);
    firebase_sql_delete_free(&stmt->del);
    free(stmt->ident);
    free(stmt->refuse_table);
    free(stmt->exists_table);
    free(stmt);
}

bool firebase_sql_set_error(FirebaseSqlStatement* stmt, const char* message) {
    if (!stmt) {
        return false;
    }
    free(stmt->error_message);
    stmt->error_message = strdup(message ? message : "SQL parse failed");
    return false;
}

bool firebase_sql_table_add_column(FirebaseSqlTable* table, FirebaseSqlColumn col) {
    FirebaseSqlColumn* grown = realloc(table->columns, (table->column_count + 1) * sizeof(FirebaseSqlColumn));
    if (!grown) {
        firebase_sql_column_free(&col);
        return false;
    }
    table->columns = grown;
    table->columns[table->column_count] = col;
    table->column_count++;
    return true;
}

bool firebase_sql_table_add_unique(FirebaseSqlTable* table, FirebaseSqlKey key) {
    FirebaseSqlKey* grown = realloc(table->unique_keys, (table->unique_count + 1) * sizeof(FirebaseSqlKey));
    if (!grown) {
        firebase_sql_key_free(&key);
        return false;
    }
    table->unique_keys = grown;
    table->unique_keys[table->unique_count] = key;
    table->unique_count++;
    return true;
}

bool firebase_sql_parse_column(const char** cursor, FirebaseSqlStatement* stmt) {
    FirebaseSqlColumn col = {0};
    if (!firebase_sql_parse_ident(cursor, &col.name)) {
        return firebase_sql_set_error(stmt, "CREATE TABLE: missing column name");
    }
    if (!firebase_sql_parse_ident(cursor, &col.type)) {
        firebase_sql_column_free(&col);
        return firebase_sql_set_error(stmt, "CREATE TABLE: missing column type");
    }
    if (!firebase_sql_type_is_known(col.type)) {
        firebase_sql_column_free(&col);
        return firebase_sql_set_error(stmt, "CREATE TABLE: unknown column type");
    }
    firebase_sql_tolower_inplace(col.type);

    while (1) {
        if (firebase_sql_match_keyword(cursor, "NOT")) {
            if (!firebase_sql_match_keyword(cursor, "NULL")) {
                firebase_sql_column_free(&col);
                return firebase_sql_set_error(stmt, "CREATE TABLE: expected NULL after NOT");
            }
            col.not_null = true;
            continue;
        }
        if (firebase_sql_match_keyword(cursor, "DEFAULT")) {
            if (firebase_sql_match_keyword(cursor, "NULL")) {
                continue;
            }
            col.default_sql = firebase_sql_parse_literal(cursor);
            if (!col.default_sql) {
                firebase_sql_column_free(&col);
                return firebase_sql_set_error(stmt, "CREATE TABLE: invalid DEFAULT");
            }
            continue;
        }
        if (firebase_sql_match_keyword(cursor, "UNIQUE")) {
            FirebaseSqlKey key = {0};
            char* copy = strdup(col.name);
            if (!copy || !firebase_sql_key_add(&key, copy) ||
                !firebase_sql_table_add_unique(&stmt->table, key)) {
                firebase_sql_column_free(&col);
                return firebase_sql_set_error(stmt, "CREATE TABLE: UNIQUE column failed");
            }
            continue;
        }
        if (firebase_sql_match_keyword(cursor, "PRIMARY")) {
            if (!firebase_sql_match_keyword(cursor, "KEY")) {
                firebase_sql_column_free(&col);
                return firebase_sql_set_error(stmt, "CREATE TABLE: expected KEY");
            }
            if (stmt->table.primary_key.count > 0) {
                firebase_sql_column_free(&col);
                return firebase_sql_set_error(stmt, "CREATE TABLE: duplicate PRIMARY KEY");
            }
            char* copy = strdup(col.name);
            if (!copy || !firebase_sql_key_add(&stmt->table.primary_key, copy)) {
                firebase_sql_column_free(&col);
                return firebase_sql_set_error(stmt, "CREATE TABLE: PRIMARY KEY failed");
            }
            continue;
        }
        break;
    }

    if (!firebase_sql_table_add_column(&stmt->table, col)) {
        return firebase_sql_set_error(stmt, "CREATE TABLE: out of memory");
    }
    return true;
}

bool firebase_sql_parse_create_table(const char** cursor, FirebaseSqlStatement* stmt) {
    stmt->kind = FIREBASE_SQL_KIND_CREATE_TABLE;
    if (!firebase_sql_parse_ident(cursor, &stmt->table.name)) {
        return firebase_sql_set_error(stmt, "CREATE TABLE: missing table name");
    }
    firebase_sql_skip(cursor);
    if (**cursor != '(') {
        return firebase_sql_set_error(stmt, "CREATE TABLE: expected '('");
    }
    (*cursor)++;

    while (1) {
        firebase_sql_skip(cursor);
        if (**cursor == ')') {
            (*cursor)++;
            break;
        }
        if (**cursor == ',') {
            (*cursor)++;
            continue;
        }
        if (firebase_sql_match_keyword(cursor, "PRIMARY")) {
            if (!firebase_sql_match_keyword(cursor, "KEY")) {
                return firebase_sql_set_error(stmt, "CREATE TABLE: expected KEY");
            }
            if (stmt->table.primary_key.count > 0) {
                return firebase_sql_set_error(stmt, "CREATE TABLE: duplicate PRIMARY KEY");
            }
            if (!firebase_sql_parse_ident_list(cursor, &stmt->table.primary_key)) {
                return firebase_sql_set_error(stmt, "CREATE TABLE: invalid PRIMARY KEY");
            }
        } else if (firebase_sql_match_keyword(cursor, "UNIQUE")) {
            FirebaseSqlKey key = {0};
            if (!firebase_sql_parse_ident_list(cursor, &key) ||
                !firebase_sql_table_add_unique(&stmt->table, key)) {
                firebase_sql_key_free(&key);
                return firebase_sql_set_error(stmt, "CREATE TABLE: invalid UNIQUE");
            }
        } else if (!firebase_sql_parse_column(cursor, stmt)) {
            return false;
        }

        firebase_sql_skip(cursor);
        if (**cursor == ',') {
            (*cursor)++;
            continue;
        }
        if (**cursor == ')') {
            (*cursor)++;
            break;
        }
        return firebase_sql_set_error(stmt, "CREATE TABLE: expected ',' or ')'");
    }

    if (stmt->table.column_count == 0) {
        return firebase_sql_set_error(stmt, "CREATE TABLE: no columns");
    }
    return true;
}

bool firebase_sql_parse_drop_table(const char** cursor, FirebaseSqlStatement* stmt) {
    stmt->kind = FIREBASE_SQL_KIND_DROP_TABLE;
    if (firebase_sql_match_keyword(cursor, "IF")) {
        if (!firebase_sql_match_keyword(cursor, "EXISTS")) {
            return firebase_sql_set_error(stmt, "DROP TABLE: expected EXISTS");
        }
        stmt->if_exists = true;
    }
    if (!firebase_sql_parse_ident(cursor, &stmt->ident)) {
        return firebase_sql_set_error(stmt, "DROP TABLE: missing table name");
    }
    return true;
}

bool firebase_sql_parse_create_index(const char** cursor, FirebaseSqlStatement* stmt, bool unique) {
    stmt->kind = FIREBASE_SQL_KIND_CREATE_INDEX;
    stmt->index.unique = unique;
    if (!firebase_sql_parse_ident(cursor, &stmt->index.name)) {
        return firebase_sql_set_error(stmt, "CREATE INDEX: missing index name");
    }
    if (!firebase_sql_match_keyword(cursor, "ON")) {
        return firebase_sql_set_error(stmt, "CREATE INDEX: expected ON");
    }
    if (!firebase_sql_parse_ident(cursor, &stmt->index.table)) {
        return firebase_sql_set_error(stmt, "CREATE INDEX: missing table name");
    }
    if (!firebase_sql_parse_ident_list(cursor, &stmt->index.columns)) {
        return firebase_sql_set_error(stmt, "CREATE INDEX: invalid column list");
    }
    return true;
}

bool firebase_sql_parse_alter_table(const char** cursor, FirebaseSqlStatement* stmt) {
    stmt->kind = FIREBASE_SQL_KIND_ALTER_TABLE;
    if (!firebase_sql_parse_ident(cursor, &stmt->alter.table)) {
        return firebase_sql_set_error(stmt, "ALTER TABLE: missing table name");
    }
    if (firebase_sql_match_keyword(cursor, "ADD")) {
        firebase_sql_match_keyword(cursor, "COLUMN");
        stmt->alter.op = FIREBASE_ALTER_ADD_COLUMN;
        FirebaseSqlStatement tmp = {0};
        if (!firebase_sql_parse_column(cursor, &tmp)) {
            stmt->error_message = tmp.error_message;
            tmp.error_message = NULL;
            firebase_sql_table_free(&tmp.table);
            return false;
        }
        if (tmp.table.column_count < 1) {
            firebase_sql_table_free(&tmp.table);
            return firebase_sql_set_error(stmt, "ALTER TABLE: ADD COLUMN failed");
        }
        stmt->alter.add_column = tmp.table.columns[0];
        tmp.table.columns[0].name = NULL;
        tmp.table.columns[0].type = NULL;
        tmp.table.columns[0].default_sql = NULL;
        firebase_sql_table_free(&tmp.table);
        return true;
    }
    if (firebase_sql_match_keyword(cursor, "DROP")) {
        firebase_sql_match_keyword(cursor, "COLUMN");
        stmt->alter.op = FIREBASE_ALTER_DROP_COLUMN;
        if (!firebase_sql_parse_ident(cursor, &stmt->alter.drop_column)) {
            return firebase_sql_set_error(stmt, "ALTER TABLE: DROP COLUMN missing name");
        }
        return true;
    }
    if (firebase_sql_match_keyword(cursor, "RENAME")) {
        if (!firebase_sql_match_keyword(cursor, "TO")) {
            return firebase_sql_set_error(stmt, "ALTER TABLE: expected TO");
        }
        stmt->alter.op = FIREBASE_ALTER_RENAME_TABLE;
        if (!firebase_sql_parse_ident(cursor, &stmt->alter.rename_to)) {
            return firebase_sql_set_error(stmt, "ALTER TABLE: RENAME missing name");
        }
        return true;
    }
    return firebase_sql_set_error(stmt, "ALTER TABLE: unsupported action");
}

bool firebase_sql_parse_create_function(const char** cursor, FirebaseSqlStatement* stmt) {
    stmt->kind = FIREBASE_SQL_KIND_CREATE_FUNCTION;
    char* name = NULL;
    if (!firebase_sql_parse_ident(cursor, &name)) {
        return firebase_sql_set_error(stmt, "CREATE FUNCTION: missing name");
    }
    firebase_sql_skip(cursor);
    if (**cursor == '.') {
        (*cursor)++;
        free(name);
        name = NULL;
        if (!firebase_sql_parse_ident(cursor, &name)) {
            return firebase_sql_set_error(stmt, "CREATE FUNCTION: missing name after schema");
        }
    }
    stmt->ident = name;
    while (**cursor && **cursor != ';') {
        (*cursor)++;
    }
    return true;
}

bool firebase_sql_parse_drop_function(const char** cursor, FirebaseSqlStatement* stmt) {
    stmt->kind = FIREBASE_SQL_KIND_DROP_FUNCTION;
    if (firebase_sql_match_keyword(cursor, "IF")) {
        if (!firebase_sql_match_keyword(cursor, "EXISTS")) {
            return firebase_sql_set_error(stmt, "DROP FUNCTION: expected EXISTS");
        }
        stmt->if_exists = true;
    }
    if (!firebase_sql_parse_ident(cursor, &stmt->ident)) {
        return firebase_sql_set_error(stmt, "DROP FUNCTION: missing name");
    }
    firebase_sql_skip(cursor);
    if (**cursor == '.') {
        (*cursor)++;
        free(stmt->ident);
        stmt->ident = NULL;
        if (!firebase_sql_parse_ident(cursor, &stmt->ident)) {
            return firebase_sql_set_error(stmt, "DROP FUNCTION: missing name after schema");
        }
    }
    while (**cursor && **cursor != ';') {
        (*cursor)++;
    }
    return true;
}

bool firebase_sql_parse_refuse_drop(const char** cursor, FirebaseSqlStatement* stmt) {
    stmt->kind = FIREBASE_SQL_KIND_REFUSE_DROP;
    firebase_sql_skip(cursor);
    if (**cursor != '(') {
        return firebase_sql_set_error(stmt, "FB_REFUSE_DROP: expected '('");
    }
    (*cursor)++;
    if (!firebase_sql_parse_string(cursor, &stmt->refuse_table)) {
        return firebase_sql_set_error(stmt, "FB_REFUSE_DROP: expected table name string");
    }
    firebase_sql_skip(cursor);
    if (**cursor != ')') {
        return firebase_sql_set_error(stmt, "FB_REFUSE_DROP: expected ')'");
    }
    (*cursor)++;
    if (!firebase_sql_match_keyword(cursor, "WHERE") ||
        !firebase_sql_match_keyword(cursor, "EXISTS")) {
        return firebase_sql_set_error(stmt, "FB_REFUSE_DROP: expected WHERE EXISTS");
    }
    firebase_sql_skip(cursor);
    if (**cursor != '(') {
        return firebase_sql_set_error(stmt, "FB_REFUSE_DROP: expected subquery '('");
    }
    (*cursor)++;
    if (!firebase_sql_match_keyword(cursor, "SELECT")) {
        return firebase_sql_set_error(stmt, "FB_REFUSE_DROP: expected SELECT");
    }
    firebase_sql_skip(cursor);
    if (**cursor == '*') {
        (*cursor)++;
    } else if (isdigit((unsigned char)**cursor)) {
        while (isdigit((unsigned char)**cursor)) {
            (*cursor)++;
        }
    } else {
        char* ignored = NULL;
        if (!firebase_sql_parse_ident(cursor, &ignored)) {
            return firebase_sql_set_error(stmt, "FB_REFUSE_DROP: expected SELECT list");
        }
        free(ignored);
    }
    if (!firebase_sql_match_keyword(cursor, "FROM")) {
        return firebase_sql_set_error(stmt, "FB_REFUSE_DROP: expected FROM");
    }
    if (!firebase_sql_parse_ident(cursor, &stmt->exists_table)) {
        return firebase_sql_set_error(stmt, "FB_REFUSE_DROP: missing FROM table");
    }
    firebase_sql_skip(cursor);
    if (**cursor != ')') {
        return firebase_sql_set_error(stmt, "FB_REFUSE_DROP: expected ')' after subquery");
    }
    (*cursor)++;
    return true;
}

bool firebase_sql_finish_statement(const char** cursor, FirebaseSqlStatement* stmt) {
    firebase_sql_skip(cursor);
    if (**cursor == ';') {
        (*cursor)++;
        firebase_sql_skip(cursor);
    }
    if (**cursor != '\0') {
        return firebase_sql_set_error(stmt, "unexpected SQL after statement");
    }
    return true;
}

FirebaseSqlStatement* firebase_sql_parse(const char* sql) {
    FirebaseSqlStatement* stmt = firebase_sql_statement_alloc();
    if (!stmt) {
        return NULL;
    }
    if (!sql) {
        firebase_sql_set_error(stmt, "SQL is NULL");
        return stmt;
    }
    const char* cursor = sql;
    firebase_sql_skip(&cursor);
    if (*cursor == '\0') {
        firebase_sql_set_error(stmt, "SQL is empty");
        return stmt;
    }

    bool ok = false;
    if (firebase_sql_match_keyword(&cursor, "CREATE")) {
        bool or_replace = false;
        if (firebase_sql_match_keyword(&cursor, "OR")) {
            if (!firebase_sql_match_keyword(&cursor, "REPLACE")) {
                firebase_sql_set_error(stmt, "CREATE: expected REPLACE");
                return stmt;
            }
            or_replace = true;
        }
        if (firebase_sql_match_keyword(&cursor, "TABLE")) {
            if (or_replace) {
                firebase_sql_set_error(stmt, "CREATE OR REPLACE TABLE is not supported");
                return stmt;
            }
            ok = firebase_sql_parse_create_table(&cursor, stmt);
        } else if (firebase_sql_match_keyword(&cursor, "UNIQUE")) {
            if (!firebase_sql_match_keyword(&cursor, "INDEX")) {
                firebase_sql_set_error(stmt, "CREATE UNIQUE: expected INDEX");
                return stmt;
            }
            ok = firebase_sql_parse_create_index(&cursor, stmt, true);
        } else if (firebase_sql_match_keyword(&cursor, "INDEX")) {
            ok = firebase_sql_parse_create_index(&cursor, stmt, false);
        } else if (firebase_sql_match_keyword(&cursor, "FUNCTION")) {
            ok = firebase_sql_parse_create_function(&cursor, stmt);
        } else {
            stmt->kind = FIREBASE_SQL_KIND_UNSUPPORTED;
            return stmt;
        }
    } else if (firebase_sql_match_keyword(&cursor, "DROP")) {
        if (firebase_sql_match_keyword(&cursor, "TABLE")) {
            ok = firebase_sql_parse_drop_table(&cursor, stmt);
        } else if (firebase_sql_match_keyword(&cursor, "FUNCTION")) {
            ok = firebase_sql_parse_drop_function(&cursor, stmt);
        } else {
            stmt->kind = FIREBASE_SQL_KIND_UNSUPPORTED;
            return stmt;
        }
    } else if (firebase_sql_match_keyword(&cursor, "ALTER")) {
        if (!firebase_sql_match_keyword(&cursor, "TABLE")) {
            firebase_sql_set_error(stmt, "ALTER: expected TABLE");
            return stmt;
        }
        ok = firebase_sql_parse_alter_table(&cursor, stmt);
    } else if (firebase_sql_match_keyword(&cursor, "SELECT")) {
        if (firebase_sql_match_keyword(&cursor, "FB_REFUSE_DROP")) {
            ok = firebase_sql_parse_refuse_drop(&cursor, stmt);
        } else {
            stmt->kind = FIREBASE_SQL_KIND_UNSUPPORTED;
            return stmt;
        }
    } else if (firebase_sql_match_keyword(&cursor, "INSERT")) {
        ok = firebase_sql_parse_insert(&cursor, stmt);
    } else if (firebase_sql_match_keyword(&cursor, "UPDATE")) {
        ok = firebase_sql_parse_update(&cursor, stmt);
    } else if (firebase_sql_match_keyword(&cursor, "DELETE")) {
        ok = firebase_sql_parse_delete(&cursor, stmt);
    } else if (firebase_sql_match_keyword(&cursor, "WITH")) {
        stmt->kind = FIREBASE_SQL_KIND_UNSUPPORTED;
        return stmt;
    } else {
        firebase_sql_set_error(stmt, "unrecognized SQL statement");
        return stmt;
    }

    if (!ok) {
        return stmt;
    }
    if (stmt->kind == FIREBASE_SQL_KIND_UNSUPPORTED) {
        return stmt;
    }
    firebase_sql_finish_statement(&cursor, stmt);
    return stmt;
}
