/*
 * Unity tests for firebase_sql_parse().
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/sql_parse.h>

void test_firebase_sql_parse_null_and_empty(void);
void test_firebase_sql_parse_create_table_queries(void);
void test_firebase_sql_parse_create_table_composite_pk(void);
void test_firebase_sql_parse_drop_and_index(void);
void test_firebase_sql_parse_alter_variants(void);
void test_firebase_sql_parse_functions_and_refuse(void);
void test_firebase_sql_parse_unsupported(void);
void test_firebase_sql_parse_column_constraints_and_comments(void);
void test_firebase_sql_helpers(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_sql_parse_null_and_empty(void) {
    FirebaseSqlStatement* stmt = firebase_sql_parse(NULL);
    TEST_ASSERT_NOT_NULL(stmt);
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("   -- just a comment\n");
    TEST_ASSERT_NOT_NULL(stmt);
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    firebase_sql_statement_free(NULL);
}

void test_firebase_sql_parse_create_table_queries(void) {
    const char* sql =
        "CREATE TABLE testfb_queries (\n"
        "    query_id integer NOT NULL,\n"
        "    query_ref integer NOT NULL,\n"
        "    query_status_a27 integer NOT NULL,\n"
        "    query_type_a28 integer NOT NULL,\n"
        "    query_dialect_a30 integer NOT NULL,\n"
        "    query_queue_a58 integer NOT NULL,\n"
        "    query_timeout integer NOT NULL,\n"
        "    code text NOT NULL,\n"
        "    name text NOT NULL,\n"
        "    summary text,\n"
        "    collection json,\n"
        "    valid_after timestamp,\n"
        "    valid_until timestamp,\n"
        "    created_id integer NOT NULL,\n"
        "    created_at timestamp NOT NULL,\n"
        "    updated_id integer NOT NULL,\n"
        "    updated_at timestamp NOT NULL,\n"
        "    PRIMARY KEY(query_id), -- Primary Key\n"
        "    UNIQUE(query_ref, query_type_a28) -- Unique Column\n"
        ");";
    FirebaseSqlStatement* stmt = firebase_sql_parse(sql);
    TEST_ASSERT_NOT_NULL(stmt);
    TEST_ASSERT_NULL(stmt->error_message);
    TEST_ASSERT_EQUAL(FIREBASE_SQL_KIND_CREATE_TABLE, stmt->kind);
    TEST_ASSERT_EQUAL_STRING("testfb_queries", stmt->table.name);
    TEST_ASSERT_EQUAL(17, stmt->table.column_count);
    TEST_ASSERT_EQUAL_STRING("query_id", stmt->table.columns[0].name);
    TEST_ASSERT_TRUE(stmt->table.columns[0].not_null);
    TEST_ASSERT_EQUAL_STRING("collection", stmt->table.columns[10].name);
    TEST_ASSERT_EQUAL_STRING("json", stmt->table.columns[10].type);
    TEST_ASSERT_FALSE(stmt->table.columns[10].not_null);
    TEST_ASSERT_EQUAL(1, stmt->table.primary_key.count);
    TEST_ASSERT_EQUAL_STRING("query_id", stmt->table.primary_key.columns[0]);
    TEST_ASSERT_EQUAL(1, stmt->table.unique_count);
    TEST_ASSERT_EQUAL(2, stmt->table.unique_keys[0].count);
    TEST_ASSERT_EQUAL_STRING("query_type_a28", stmt->table.unique_keys[0].columns[1]);
    firebase_sql_statement_free(stmt);
}

void test_firebase_sql_parse_create_table_composite_pk(void) {
    const char* sql =
        "CREATE TABLE testfb_lookups (\n"
        "    lookup_id integer NOT NULL,\n"
        "    key_idx integer NOT NULL,\n"
        "    access_count integer DEFAULT 0,\n"
        "    metadata json DEFAULT '{}',\n"
        "    PRIMARY KEY(lookup_id, key_idx)\n"
        ")";
    FirebaseSqlStatement* stmt = firebase_sql_parse(sql);
    TEST_ASSERT_NOT_NULL(stmt);
    TEST_ASSERT_NULL(stmt->error_message);
    TEST_ASSERT_EQUAL(2, stmt->table.primary_key.count);
    TEST_ASSERT_EQUAL_STRING("0", stmt->table.columns[2].default_sql);
    TEST_ASSERT_EQUAL_STRING("'{}'", stmt->table.columns[3].default_sql);
    firebase_sql_statement_free(stmt);
}

void test_firebase_sql_parse_drop_and_index(void) {
    FirebaseSqlStatement* stmt = firebase_sql_parse("DROP TABLE testfb_lookups;");
    TEST_ASSERT_EQUAL(FIREBASE_SQL_KIND_DROP_TABLE, stmt->kind);
    TEST_ASSERT_EQUAL_STRING("testfb_lookups", stmt->ident);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("DROP TABLE IF EXISTS missing");
    TEST_ASSERT_TRUE(stmt->if_exists);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse(
        "CREATE INDEX queries_idx_last_accessed ON testfb_queries(last_accessed);");
    TEST_ASSERT_EQUAL(FIREBASE_SQL_KIND_CREATE_INDEX, stmt->kind);
    TEST_ASSERT_FALSE(stmt->index.unique);
    TEST_ASSERT_EQUAL_STRING("testfb_queries", stmt->index.table);
    TEST_ASSERT_EQUAL(1, stmt->index.columns.count);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("CREATE UNIQUE INDEX uq ON testfb_queries (query_ref, query_type_a28)");
    TEST_ASSERT_TRUE(stmt->index.unique);
    TEST_ASSERT_EQUAL(2, stmt->index.columns.count);
    firebase_sql_statement_free(stmt);
}

void test_firebase_sql_parse_alter_variants(void) {
    FirebaseSqlStatement* stmt = firebase_sql_parse(
        "ALTER TABLE testfb_scripts ADD COLUMN invokable integer NOT NULL DEFAULT 0;");
    TEST_ASSERT_EQUAL(FIREBASE_SQL_KIND_ALTER_TABLE, stmt->kind);
    TEST_ASSERT_EQUAL(FIREBASE_ALTER_ADD_COLUMN, stmt->alter.op);
    TEST_ASSERT_EQUAL_STRING("invokable", stmt->alter.add_column.name);
    TEST_ASSERT_TRUE(stmt->alter.add_column.not_null);
    TEST_ASSERT_EQUAL_STRING("0", stmt->alter.add_column.default_sql);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("ALTER TABLE testfb_scripts DROP COLUMN invokable;");
    TEST_ASSERT_EQUAL(FIREBASE_ALTER_DROP_COLUMN, stmt->alter.op);
    TEST_ASSERT_EQUAL_STRING("invokable", stmt->alter.drop_column);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("ALTER TABLE testfb_accounts_new RENAME TO accounts;");
    TEST_ASSERT_EQUAL(FIREBASE_ALTER_RENAME_TABLE, stmt->alter.op);
    TEST_ASSERT_EQUAL_STRING("testfb_accounts_new", stmt->alter.table);
    TEST_ASSERT_EQUAL_STRING("accounts", stmt->alter.rename_to);
    firebase_sql_statement_free(stmt);
}

void test_firebase_sql_parse_functions_and_refuse(void) {
    FirebaseSqlStatement* stmt = firebase_sql_parse(
        "CREATE FUNCTION FB_JSON_INGEST (x text) RETURNS text");
    TEST_ASSERT_EQUAL(FIREBASE_SQL_KIND_CREATE_FUNCTION, stmt->kind);
    TEST_ASSERT_EQUAL_STRING("FB_JSON_INGEST", stmt->ident);
    TEST_ASSERT_TRUE(firebase_sql_is_known_function(stmt->ident));
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("CREATE OR REPLACE FUNCTION FB_NOW()");
    TEST_ASSERT_EQUAL(FIREBASE_SQL_KIND_CREATE_FUNCTION, stmt->kind);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("DROP FUNCTION IF EXISTS brotli_decompress;");
    TEST_ASSERT_EQUAL(FIREBASE_SQL_KIND_DROP_FUNCTION, stmt->kind);
    TEST_ASSERT_TRUE(stmt->if_exists);
    TEST_ASSERT_FALSE(firebase_sql_is_known_function(stmt->ident));
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse(
        "SELECT FB_REFUSE_DROP('testfb_lookups') WHERE EXISTS (SELECT 1 FROM testfb_lookups)");
    TEST_ASSERT_NULL(stmt->error_message);
    TEST_ASSERT_EQUAL(FIREBASE_SQL_KIND_REFUSE_DROP, stmt->kind);
    TEST_ASSERT_EQUAL_STRING("testfb_lookups", stmt->refuse_table);
    TEST_ASSERT_EQUAL_STRING("testfb_lookups", stmt->exists_table);
    firebase_sql_statement_free(stmt);
}

void test_firebase_sql_parse_unsupported(void) {
    FirebaseSqlStatement* stmt = firebase_sql_parse("SELECT 1");
    TEST_ASSERT_EQUAL(FIREBASE_SQL_KIND_UNSUPPORTED, stmt->kind);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("INSERT INTO t VALUES (1)");
    TEST_ASSERT_EQUAL(FIREBASE_SQL_KIND_UNSUPPORTED, stmt->kind);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("INSERT INTO t (a) SELECT 1");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("CREATE VIEW v AS SELECT 1");
    TEST_ASSERT_EQUAL(FIREBASE_SQL_KIND_UNSUPPORTED, stmt->kind);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("CREATE TABLE t (x widget)");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("not sql at all");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);
}

void test_firebase_sql_parse_column_constraints_and_comments(void) {
    FirebaseSqlStatement* stmt = firebase_sql_parse(
        "/* leading */ CREATE TABLE t (\n"
        "  id integer PRIMARY KEY,\n"
        "  code text UNIQUE,\n"
        "  note text DEFAULT NULL\n"
        ")");
    TEST_ASSERT_NULL(stmt->error_message);
    TEST_ASSERT_EQUAL(1, stmt->table.primary_key.count);
    TEST_ASSERT_EQUAL_STRING("id", stmt->table.primary_key.columns[0]);
    TEST_ASSERT_EQUAL(1, stmt->table.unique_count);
    TEST_ASSERT_EQUAL_STRING("code", stmt->table.unique_keys[0].columns[0]);
    TEST_ASSERT_NULL(stmt->table.columns[2].default_sql);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("CREATE OR REPLACE TABLE t (id integer)");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("CREATE TABLE t (id integer); SELECT 1");
    TEST_ASSERT_NOT_NULL(stmt->error_message);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("CREATE FUNCTION app.FB_NOW()");
    TEST_ASSERT_EQUAL_STRING("FB_NOW", stmt->ident);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("DROP FUNCTION app.FB_NOW");
    TEST_ASSERT_EQUAL_STRING("FB_NOW", stmt->ident);
    firebase_sql_statement_free(stmt);

    stmt = firebase_sql_parse("SELECT FB_REFUSE_DROP('t') WHERE EXISTS (SELECT * FROM t)");
    TEST_ASSERT_EQUAL(FIREBASE_SQL_KIND_REFUSE_DROP, stmt->kind);
    firebase_sql_statement_free(stmt);
}

void test_firebase_sql_helpers(void) {
    const char* p = "  -- c\n  CREATE TABLE";
    TEST_ASSERT_TRUE(firebase_sql_match_keyword(&p, "CREATE"));
    TEST_ASSERT_TRUE(firebase_sql_type_is_known("INTEGER"));
    TEST_ASSERT_FALSE(firebase_sql_type_is_known("blob"));
    TEST_ASSERT_TRUE(firebase_sql_is_known_function("fb_now"));
    TEST_ASSERT_FALSE(firebase_sql_is_known_function("sha256"));

    char typebuf[] = "TIMESTAMP";
    firebase_sql_tolower_inplace(typebuf);
    TEST_ASSERT_EQUAL_STRING("timestamp", typebuf);
    firebase_sql_tolower_inplace(NULL);

    const char* s = "'it''s'";
    char* inner = NULL;
    TEST_ASSERT_TRUE(firebase_sql_parse_string(&s, &inner));
    TEST_ASSERT_EQUAL_STRING("it's", inner);
    free(inner);

    const char* lit = "  -12.5";
    char* number = firebase_sql_parse_literal(&lit);
    TEST_ASSERT_EQUAL_STRING("-12.5", number);
    free(number);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_sql_parse_null_and_empty);
    RUN_TEST(test_firebase_sql_parse_create_table_queries);
    RUN_TEST(test_firebase_sql_parse_create_table_composite_pk);
    RUN_TEST(test_firebase_sql_parse_drop_and_index);
    RUN_TEST(test_firebase_sql_parse_alter_variants);
    RUN_TEST(test_firebase_sql_parse_functions_and_refuse);
    RUN_TEST(test_firebase_sql_parse_unsupported);
    RUN_TEST(test_firebase_sql_parse_column_constraints_and_comments);
    RUN_TEST(test_firebase_sql_helpers);
    return UNITY_END();
}
