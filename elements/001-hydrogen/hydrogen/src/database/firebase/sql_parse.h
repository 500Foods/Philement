/*
 * Firebase SQL subset parser (Acuranzo DDL + DROP_CHECK).
 */

#ifndef DATABASE_ENGINE_FIREBASE_SQL_PARSE_H
#define DATABASE_ENGINE_FIREBASE_SQL_PARSE_H

#include <stddef.h>
#include <stdbool.h>
#include "sql_expr.h"

typedef enum {
    FIREBASE_SQL_KIND_NONE = 0,
    FIREBASE_SQL_KIND_CREATE_TABLE,
    FIREBASE_SQL_KIND_DROP_TABLE,
    FIREBASE_SQL_KIND_CREATE_INDEX,
    FIREBASE_SQL_KIND_ALTER_TABLE,
    FIREBASE_SQL_KIND_CREATE_FUNCTION,
    FIREBASE_SQL_KIND_DROP_FUNCTION,
    FIREBASE_SQL_KIND_REFUSE_DROP,
    FIREBASE_SQL_KIND_INSERT,
    FIREBASE_SQL_KIND_UPDATE,
    FIREBASE_SQL_KIND_DELETE,
    FIREBASE_SQL_KIND_UNSUPPORTED
} FirebaseSqlKind;

typedef enum {
    FIREBASE_ALTER_NONE = 0,
    FIREBASE_ALTER_ADD_COLUMN,
    FIREBASE_ALTER_DROP_COLUMN,
    FIREBASE_ALTER_RENAME_TABLE
} FirebaseAlterOp;

typedef struct FirebaseSqlColumn {
    char* name;
    char* type;
    bool not_null;
    char* default_sql;
} FirebaseSqlColumn;

typedef struct FirebaseSqlKey {
    char** columns;
    size_t count;
} FirebaseSqlKey;

typedef struct FirebaseSqlTable {
    char* name;
    FirebaseSqlColumn* columns;
    size_t column_count;
    FirebaseSqlKey primary_key;
    FirebaseSqlKey* unique_keys;
    size_t unique_count;
} FirebaseSqlTable;

typedef struct FirebaseSqlIndex {
    char* name;
    char* table;
    bool unique;
    FirebaseSqlKey columns;
} FirebaseSqlIndex;

typedef struct FirebaseSqlAlter {
    char* table;
    FirebaseAlterOp op;
    FirebaseSqlColumn add_column;
    char* drop_column;
    char* rename_to;
} FirebaseSqlAlter;

typedef enum {
    FIREBASE_PRED_EQ = 0,
    FIREBASE_PRED_IS_NULL,
    FIREBASE_PRED_IN
} FirebaseSqlPredOp;

typedef struct FirebaseSqlPredicate {
    char* column;
    FirebaseSqlPredOp op;
    FirebaseExpr* value;
    FirebaseExpr** in_values;
    size_t in_count;
} FirebaseSqlPredicate;

typedef struct FirebaseSqlWhere {
    FirebaseSqlPredicate* predicates;
    size_t count;
} FirebaseSqlWhere;

typedef struct FirebaseSqlInsertRow {
    FirebaseExpr** values;
    size_t count;
} FirebaseSqlInsertRow;

typedef struct FirebaseSqlInsert {
    char* table;
    char** columns;
    size_t column_count;
    FirebaseSqlInsertRow* rows;
    size_t row_count;
} FirebaseSqlInsert;

typedef struct FirebaseSqlAssign {
    char* column;
    FirebaseExpr* value;
} FirebaseSqlAssign;

typedef struct FirebaseSqlUpdate {
    char* table;
    FirebaseSqlAssign* sets;
    size_t set_count;
    FirebaseSqlWhere where;
} FirebaseSqlUpdate;

typedef struct FirebaseSqlDelete {
    char* table;
    FirebaseSqlWhere where;
} FirebaseSqlDelete;

typedef struct FirebaseSqlStatement {
    FirebaseSqlKind kind;
    char* error_message;
    bool if_exists;
    FirebaseSqlTable table;
    FirebaseSqlIndex index;
    FirebaseSqlAlter alter;
    FirebaseSqlInsert insert;
    FirebaseSqlUpdate update;
    FirebaseSqlDelete del;
    char* ident;
    char* refuse_table;
    char* exists_table;
} FirebaseSqlStatement;

void firebase_sql_skip(const char** cursor);
bool firebase_sql_match_keyword(const char** cursor, const char* keyword);
bool firebase_sql_parse_ident(const char** cursor, char** out);
bool firebase_sql_parse_string(const char** cursor, char** out);
char* firebase_sql_parse_literal(const char** cursor);
bool firebase_sql_type_is_known(const char* type);
void firebase_sql_tolower_inplace(char* text);
bool firebase_sql_is_known_function(const char* name);

void firebase_sql_key_free(FirebaseSqlKey* key);
bool firebase_sql_key_add(FirebaseSqlKey* key, char* ident);
bool firebase_sql_parse_ident_list(const char** cursor, FirebaseSqlKey* key);
void firebase_sql_column_free(FirebaseSqlColumn* col);
void firebase_sql_table_free(FirebaseSqlTable* table);
bool firebase_sql_set_error(FirebaseSqlStatement* stmt, const char* message);
bool firebase_sql_table_add_column(FirebaseSqlTable* table, FirebaseSqlColumn col);
bool firebase_sql_table_add_unique(FirebaseSqlTable* table, FirebaseSqlKey key);
bool firebase_sql_parse_column(const char** cursor, FirebaseSqlStatement* stmt);

FirebaseSqlStatement* firebase_sql_statement_alloc(void);
void firebase_sql_statement_free(FirebaseSqlStatement* stmt);
FirebaseSqlStatement* firebase_sql_parse(const char* sql);

bool firebase_sql_parse_create_table(const char** cursor, FirebaseSqlStatement* stmt);
bool firebase_sql_parse_drop_table(const char** cursor, FirebaseSqlStatement* stmt);
bool firebase_sql_parse_create_index(const char** cursor, FirebaseSqlStatement* stmt, bool unique);
bool firebase_sql_parse_alter_table(const char** cursor, FirebaseSqlStatement* stmt);
bool firebase_sql_parse_create_function(const char** cursor, FirebaseSqlStatement* stmt);
bool firebase_sql_parse_drop_function(const char** cursor, FirebaseSqlStatement* stmt);
bool firebase_sql_parse_refuse_drop(const char** cursor, FirebaseSqlStatement* stmt);
bool firebase_sql_parse_insert(const char** cursor, FirebaseSqlStatement* stmt);
bool firebase_sql_parse_update(const char** cursor, FirebaseSqlStatement* stmt);
bool firebase_sql_parse_delete(const char** cursor, FirebaseSqlStatement* stmt);
bool firebase_sql_parse_where(const char** cursor, FirebaseSqlWhere* where, FirebaseSqlStatement* stmt);
bool firebase_sql_where_add(FirebaseSqlWhere* where, FirebaseSqlPredicate pred);
bool firebase_sql_finish_statement(const char** cursor, FirebaseSqlStatement* stmt);

void firebase_sql_where_free(FirebaseSqlWhere* where);
void firebase_sql_insert_free(FirebaseSqlInsert* insert);
void firebase_sql_update_free(FirebaseSqlUpdate* update);
void firebase_sql_delete_free(FirebaseSqlDelete* del);
bool firebase_sql_kind_is_dml(FirebaseSqlKind kind);

#endif /* DATABASE_ENGINE_FIREBASE_SQL_PARSE_H */
