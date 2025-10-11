/*
 * Unity Test File: db2_remove_prepared_statement
 * Tests for DB2 prepared statement cache removal
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include DB2 types and prepared statement functions
#include <src/database/db2/types.h>
#include <src/database/db2/prepared.h>

// Forward declaration
bool db2_remove_prepared_statement(PreparedStatementCache* cache, const char* name);

// Function prototypes
void test_remove_prepared_statement_null_cache(void);
void test_remove_prepared_statement_null_name(void);
void test_remove_prepared_statement_empty_cache(void);
void test_remove_prepared_statement_not_found(void);
void test_remove_prepared_statement_first(void);
void test_remove_prepared_statement_middle(void);
void test_remove_prepared_statement_last(void);
void test_remove_prepared_statement_single(void);
void test_remove_prepared_statement_multiple_sequential(void);

void setUp(void) {
    // Setup before each test
}

void tearDown(void) {
    // Cleanup after each test
}

// Test: NULL cache parameter
void test_remove_prepared_statement_null_cache(void) {
    bool result = db2_remove_prepared_statement(NULL, "test_stmt");
    TEST_ASSERT_FALSE(result);
}

// Test: NULL name parameter
void test_remove_prepared_statement_null_name(void) {
    PreparedStatementCache cache = {0};
    cache.names = calloc(10, sizeof(char*));
    cache.capacity = 10;
    cache.count = 0;
    pthread_mutex_init(&cache.lock, NULL);
    
    bool result = db2_remove_prepared_statement(&cache, NULL);
    TEST_ASSERT_FALSE(result);
    
    pthread_mutex_destroy(&cache.lock);
    free(cache.names);
}

// Test: Remove from empty cache
void test_remove_prepared_statement_empty_cache(void) {
    PreparedStatementCache cache = {0};
    cache.names = calloc(10, sizeof(char*));
    cache.capacity = 10;
    cache.count = 0;
    pthread_mutex_init(&cache.lock, NULL);
    
    bool result = db2_remove_prepared_statement(&cache, "nonexistent");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0, cache.count);
    
    pthread_mutex_destroy(&cache.lock);
    free(cache.names);
}

// Test: Remove non-existent statement
void test_remove_prepared_statement_not_found(void) {
    PreparedStatementCache cache = {0};
    cache.names = calloc(10, sizeof(char*));
    cache.capacity = 10;
    cache.count = 2;
    cache.names[0] = strdup("stmt_1");
    cache.names[1] = strdup("stmt_2");
    pthread_mutex_init(&cache.lock, NULL);
    
    bool result = db2_remove_prepared_statement(&cache, "stmt_3");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(2, cache.count);
    
    // Cleanup
    free(cache.names[0]);
    free(cache.names[1]);
    pthread_mutex_destroy(&cache.lock);
    free(cache.names);
}

// Test: Remove first statement
void test_remove_prepared_statement_first(void) {
    PreparedStatementCache cache = {0};
    cache.names = calloc(10, sizeof(char*));
    cache.capacity = 10;
    cache.count = 3;
    cache.names[0] = strdup("stmt_1");
    cache.names[1] = strdup("stmt_2");
    cache.names[2] = strdup("stmt_3");
    pthread_mutex_init(&cache.lock, NULL);
    
    bool result = db2_remove_prepared_statement(&cache, "stmt_1");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, cache.count);
    TEST_ASSERT_EQUAL_STRING("stmt_2", cache.names[0]);
    TEST_ASSERT_EQUAL_STRING("stmt_3", cache.names[1]);
    
    // Cleanup
    free(cache.names[0]);
    free(cache.names[1]);
    pthread_mutex_destroy(&cache.lock);
    free(cache.names);
}

// Test: Remove middle statement
void test_remove_prepared_statement_middle(void) {
    PreparedStatementCache cache = {0};
    cache.names = calloc(10, sizeof(char*));
    cache.capacity = 10;
    cache.count = 3;
    cache.names[0] = strdup("stmt_1");
    cache.names[1] = strdup("stmt_2");
    cache.names[2] = strdup("stmt_3");
    pthread_mutex_init(&cache.lock, NULL);
    
    bool result = db2_remove_prepared_statement(&cache, "stmt_2");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, cache.count);
    TEST_ASSERT_EQUAL_STRING("stmt_1", cache.names[0]);
    TEST_ASSERT_EQUAL_STRING("stmt_3", cache.names[1]);
    
    // Cleanup
    free(cache.names[0]);
    free(cache.names[1]);
    pthread_mutex_destroy(&cache.lock);
    free(cache.names);
}

// Test: Remove last statement
void test_remove_prepared_statement_last(void) {
    PreparedStatementCache cache = {0};
    cache.names = calloc(10, sizeof(char*));
    cache.capacity = 10;
    cache.count = 3;
    cache.names[0] = strdup("stmt_1");
    cache.names[1] = strdup("stmt_2");
    cache.names[2] = strdup("stmt_3");
    pthread_mutex_init(&cache.lock, NULL);
    
    bool result = db2_remove_prepared_statement(&cache, "stmt_3");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, cache.count);
    TEST_ASSERT_EQUAL_STRING("stmt_1", cache.names[0]);
    TEST_ASSERT_EQUAL_STRING("stmt_2", cache.names[1]);
    
    // Cleanup
    free(cache.names[0]);
    free(cache.names[1]);
    pthread_mutex_destroy(&cache.lock);
    free(cache.names);
}

// Test: Remove only statement in cache
void test_remove_prepared_statement_single(void) {
    PreparedStatementCache cache = {0};
    cache.names = calloc(10, sizeof(char*));
    cache.capacity = 10;
    cache.count = 1;
    cache.names[0] = strdup("only_stmt");
    pthread_mutex_init(&cache.lock, NULL);
    
    bool result = db2_remove_prepared_statement(&cache, "only_stmt");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, cache.count);
    
    pthread_mutex_destroy(&cache.lock);
    free(cache.names);
}

// Test: Remove multiple statements sequentially
void test_remove_prepared_statement_multiple_sequential(void) {
    PreparedStatementCache cache = {0};
    cache.names = calloc(10, sizeof(char*));
    cache.capacity = 10;
    cache.count = 5;
    cache.names[0] = strdup("stmt_1");
    cache.names[1] = strdup("stmt_2");
    cache.names[2] = strdup("stmt_3");
    cache.names[3] = strdup("stmt_4");
    cache.names[4] = strdup("stmt_5");
    pthread_mutex_init(&cache.lock, NULL);
    
    // Remove stmt_2
    TEST_ASSERT_TRUE(db2_remove_prepared_statement(&cache, "stmt_2"));
    TEST_ASSERT_EQUAL(4, cache.count);
    
    // Remove stmt_4
    TEST_ASSERT_TRUE(db2_remove_prepared_statement(&cache, "stmt_4"));
    TEST_ASSERT_EQUAL(3, cache.count);
    
    // Verify remaining statements
    TEST_ASSERT_EQUAL_STRING("stmt_1", cache.names[0]);
    TEST_ASSERT_EQUAL_STRING("stmt_3", cache.names[1]);
    TEST_ASSERT_EQUAL_STRING("stmt_5", cache.names[2]);
    
    // Cleanup
    for (size_t i = 0; i < cache.count; i++) {
        free(cache.names[i]);
    }
    pthread_mutex_destroy(&cache.lock);
    free(cache.names);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_remove_prepared_statement_null_cache);
    RUN_TEST(test_remove_prepared_statement_null_name);
    RUN_TEST(test_remove_prepared_statement_empty_cache);
    RUN_TEST(test_remove_prepared_statement_not_found);
    RUN_TEST(test_remove_prepared_statement_first);
    RUN_TEST(test_remove_prepared_statement_middle);
    RUN_TEST(test_remove_prepared_statement_last);
    RUN_TEST(test_remove_prepared_statement_single);
    RUN_TEST(test_remove_prepared_statement_multiple_sequential);
    
    return UNITY_END();
}