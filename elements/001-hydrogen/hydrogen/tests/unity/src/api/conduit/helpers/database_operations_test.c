/*
 * Conduit Database Operations Helper Test
 *
 * Tests statement-type restrictions and protected query lookup helpers.
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <src/api/conduit/conduit_helpers.h>

// Forward declarations for functions being tested
bool query_statement_type_allowed(int query_type, const char* sql_template);
bool lookup_database_and_protected_query(DatabaseQueue** db_queue,
                                         QueryCacheEntry** cache_entry,
                                         const char* database,
                                         int query_ref);

// Forward declarations for test functions
void test_query_statement_type_allowed_public_select(void);
void test_query_statement_type_allowed_public_insert(void);
void test_query_statement_type_allowed_public_whitespace(void);
void test_query_statement_type_allowed_protected_dml(void);
void test_query_statement_type_allowed_protected_ddl(void);
void test_query_statement_type_allowed_null_sql(void);
void test_query_statement_type_allowed_other_types(void);
void test_lookup_protected_query_null_params(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_query_statement_type_allowed_public_select(void) {
    TEST_ASSERT_TRUE(query_statement_type_allowed(10, "SELECT * FROM users"));
    TEST_ASSERT_TRUE(query_statement_type_allowed(10, "select id from accounts"));
}

void test_query_statement_type_allowed_public_insert(void) {
    TEST_ASSERT_FALSE(query_statement_type_allowed(10, "INSERT INTO users VALUES (1)"));
    TEST_ASSERT_FALSE(query_statement_type_allowed(10, "UPDATE users SET x=1"));
    TEST_ASSERT_FALSE(query_statement_type_allowed(10, "DELETE FROM users"));
}

void test_query_statement_type_allowed_public_whitespace(void) {
    TEST_ASSERT_TRUE(query_statement_type_allowed(10, "  \n\tSELECT * FROM users"));
    TEST_ASSERT_FALSE(query_statement_type_allowed(10, "  \n\tINSERT INTO users VALUES (1)"));
}

void test_query_statement_type_allowed_protected_dml(void) {
    TEST_ASSERT_TRUE(query_statement_type_allowed(11, "SELECT * FROM users"));
    TEST_ASSERT_TRUE(query_statement_type_allowed(11, "INSERT INTO users VALUES (1)"));
    TEST_ASSERT_TRUE(query_statement_type_allowed(11, "UPDATE users SET x=1"));
    TEST_ASSERT_TRUE(query_statement_type_allowed(11, "DELETE FROM users"));
}

void test_query_statement_type_allowed_protected_ddl(void) {
    TEST_ASSERT_FALSE(query_statement_type_allowed(11, "CREATE TABLE users (id INT)"));
    TEST_ASSERT_FALSE(query_statement_type_allowed(11, "DROP TABLE users"));
    TEST_ASSERT_FALSE(query_statement_type_allowed(11, "ALTER TABLE users ADD col INT"));
}

void test_query_statement_type_allowed_null_sql(void) {
    TEST_ASSERT_FALSE(query_statement_type_allowed(10, NULL));
    TEST_ASSERT_FALSE(query_statement_type_allowed(11, NULL));
}

void test_query_statement_type_allowed_other_types(void) {
    // Types other than 10/11 are not validated and return true
    TEST_ASSERT_TRUE(query_statement_type_allowed(0, "CREATE TABLE users (id INT)"));
    TEST_ASSERT_TRUE(query_statement_type_allowed(999, "DROP TABLE users"));
}

void test_lookup_protected_query_null_params(void) {
    DatabaseQueue* db_queue = (DatabaseQueue*)0x12345678;
    QueryCacheEntry* cache_entry = (QueryCacheEntry*)0x87654321;

    TEST_ASSERT_FALSE(lookup_database_and_protected_query(NULL, &cache_entry, "db", 1));
    TEST_ASSERT_FALSE(lookup_database_and_protected_query(&db_queue, NULL, "db", 1));
    TEST_ASSERT_FALSE(lookup_database_and_protected_query(&db_queue, &cache_entry, NULL, 1));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_query_statement_type_allowed_public_select);
    RUN_TEST(test_query_statement_type_allowed_public_insert);
    RUN_TEST(test_query_statement_type_allowed_public_whitespace);
    RUN_TEST(test_query_statement_type_allowed_protected_dml);
    RUN_TEST(test_query_statement_type_allowed_protected_ddl);
    RUN_TEST(test_query_statement_type_allowed_null_sql);
    RUN_TEST(test_query_statement_type_allowed_other_types);
    RUN_TEST(test_lookup_protected_query_null_params);
    return UNITY_END();
}
