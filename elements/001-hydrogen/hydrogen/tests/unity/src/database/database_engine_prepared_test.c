/*
 * Unity Test File: database_engine_prepared_test
 * This file contains unit tests for the prepared-statement cache helpers
 * (defined in src/database/database_engine_prepared.c):
 *   - find_prepared_statement()
 *   - store_prepared_statement()
 *   - database_engine_clear_prepared_statements()
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>

// Test function prototypes
void test_find_prepared_statement_null_connection(void);
void test_find_prepared_statement_null_name(void);
void test_find_prepared_statement_no_cache(void);
void test_store_prepared_statement_null_connection(void);
void test_store_prepared_statement_null_stmt(void);
void test_store_and_find_prepared_statement(void);
void test_store_prepared_statement_eviction(void);
void test_clear_prepared_statements_null_connection(void);
void test_clear_prepared_statements_no_cache(void);
void test_clear_prepared_statements_basic(void);

// Shared fixtures
static ConnectionConfig test_config;
static DatabaseHandle test_conn;

// Helper: build a heap-allocated prepared statement with the given name
static PreparedStatement* make_stmt(const char* name) {
    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    if (!stmt) return NULL;
    stmt->name = strdup(name);
    stmt->sql_template = strdup("SELECT 1");
    return stmt;
}

void setUp(void) {
    memset(&test_config, 0, sizeof(test_config));
    test_config.prepared_statement_cache_size = 3;

    memset(&test_conn, 0, sizeof(test_conn));
    test_conn.engine_type = DB_ENGINE_AI; // no engine registered -> fallback free path
    test_conn.config = &test_config;
    test_conn.designator = NULL;
    test_conn.prepared_statements = NULL;
    test_conn.prepared_statement_count = 0;
}

void tearDown(void) {
    // Free any remaining cached statements and the cache arrays
    if (test_conn.prepared_statements) {
        for (size_t i = 0; i < test_conn.prepared_statement_count; i++) {
            PreparedStatement* stmt = test_conn.prepared_statements[i];
            if (stmt) {
                if (stmt->name) free(stmt->name);
                if (stmt->sql_template) free(stmt->sql_template);
                free(stmt);
            }
        }
        free(test_conn.prepared_statements);
        test_conn.prepared_statements = NULL;
    }
    if (test_conn.prepared_statement_lru_counter) {
        free(test_conn.prepared_statement_lru_counter);
        test_conn.prepared_statement_lru_counter = NULL;
    }
    test_conn.prepared_statement_count = 0;
}

// find_prepared_statement

void test_find_prepared_statement_null_connection(void) {
    TEST_ASSERT_NULL(find_prepared_statement(NULL, "stmt"));
}

void test_find_prepared_statement_null_name(void) {
    TEST_ASSERT_NULL(find_prepared_statement(&test_conn, NULL));
}

void test_find_prepared_statement_no_cache(void) {
    // No prepared_statements array allocated yet
    TEST_ASSERT_NULL(find_prepared_statement(&test_conn, "stmt"));
}

// store_prepared_statement

void test_store_prepared_statement_null_connection(void) {
    PreparedStatement* stmt = make_stmt("s1");
    TEST_ASSERT_NOT_NULL(stmt);
    TEST_ASSERT_FALSE(store_prepared_statement(NULL, stmt));
    // Clean up (not stored anywhere)
    free(stmt->name);
    free(stmt->sql_template);
    free(stmt);
}

void test_store_prepared_statement_null_stmt(void) {
    TEST_ASSERT_FALSE(store_prepared_statement(&test_conn, NULL));
}

void test_store_and_find_prepared_statement(void) {
    PreparedStatement* stmt = make_stmt("lookup_me");
    TEST_ASSERT_NOT_NULL(stmt);

    bool ok = store_prepared_statement(&test_conn, stmt);
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_EQUAL_UINT(1, (unsigned int)test_conn.prepared_statement_count);

    PreparedStatement* found = find_prepared_statement(&test_conn, "lookup_me");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_PTR(stmt, found);

    // Not found for a different name
    TEST_ASSERT_NULL(find_prepared_statement(&test_conn, "other"));
}

void test_store_prepared_statement_eviction(void) {
    // cache_size is 3; store 4 to trigger LRU eviction of the first
    TEST_ASSERT_TRUE(store_prepared_statement(&test_conn, make_stmt("s1")));
    TEST_ASSERT_TRUE(store_prepared_statement(&test_conn, make_stmt("s2")));
    TEST_ASSERT_TRUE(store_prepared_statement(&test_conn, make_stmt("s3")));
    TEST_ASSERT_EQUAL_UINT(3, (unsigned int)test_conn.prepared_statement_count);

    // Fourth store: cache is full, LRU eviction path runs (no engine -> fallback free)
    TEST_ASSERT_TRUE(store_prepared_statement(&test_conn, make_stmt("s4")));

    // Count should remain within cache size
    TEST_ASSERT_TRUE(test_conn.prepared_statement_count <= 3);

    // The newest statement should be findable
    TEST_ASSERT_NOT_NULL(find_prepared_statement(&test_conn, "s4"));
}

// database_engine_clear_prepared_statements

void test_clear_prepared_statements_null_connection(void) {
    // Should not crash
    database_engine_clear_prepared_statements(NULL);
}

void test_clear_prepared_statements_no_cache(void) {
    // No allocated cache -> should be a no-op and not crash
    database_engine_clear_prepared_statements(&test_conn);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned int)test_conn.prepared_statement_count);
}

void test_clear_prepared_statements_basic(void) {
    TEST_ASSERT_TRUE(store_prepared_statement(&test_conn, make_stmt("s1")));
    TEST_ASSERT_TRUE(store_prepared_statement(&test_conn, make_stmt("s2")));
    TEST_ASSERT_EQUAL_UINT(2, (unsigned int)test_conn.prepared_statement_count);

    database_engine_clear_prepared_statements(&test_conn);

    // Clear NULLs out the pointers and resets the count (intentional leak of stmt bodies)
    TEST_ASSERT_EQUAL_UINT(0, (unsigned int)test_conn.prepared_statement_count);
    TEST_ASSERT_NULL(find_prepared_statement(&test_conn, "s1"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_find_prepared_statement_null_connection);
    RUN_TEST(test_find_prepared_statement_null_name);
    RUN_TEST(test_find_prepared_statement_no_cache);

    RUN_TEST(test_store_prepared_statement_null_connection);
    RUN_TEST(test_store_prepared_statement_null_stmt);
    RUN_TEST(test_store_and_find_prepared_statement);
    RUN_TEST(test_store_prepared_statement_eviction);

    RUN_TEST(test_clear_prepared_statements_null_connection);
    RUN_TEST(test_clear_prepared_statements_no_cache);
    RUN_TEST(test_clear_prepared_statements_basic);

    return UNITY_END();
}
