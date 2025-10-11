/*
 * Unity Test File: db2_add_prepared_statement
 * Tests for DB2 prepared statement cache addition
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include DB2 types and prepared statement functions
#include <src/database/db2/types.h>
#include <src/database/db2/prepared.h>

// Forward declaration
bool db2_add_prepared_statement(PreparedStatementCache* cache, const char* name);

// Function prototypes
void test_add_prepared_statement_null_cache(void);
void test_add_prepared_statement_null_name(void);
void test_add_prepared_statement_empty_cache(void);
void test_add_prepared_statement_duplicate(void);
void test_add_prepared_statement_multiple(void);
void test_add_prepared_statement_capacity_expansion(void);
void test_add_prepared_statement_long_name(void);

void setUp(void) {
    // Setup before each test
}

void tearDown(void) {
    // Cleanup after each test
}

// Test: NULL cache parameter
void test_add_prepared_statement_null_cache(void) {
    bool result = db2_add_prepared_statement(NULL, "test_stmt");
    TEST_ASSERT_FALSE(result);
}

// Test: NULL name parameter
void test_add_prepared_statement_null_name(void) {
    PreparedStatementCache cache = {0};
    cache.names = calloc(10, sizeof(char*));
    cache.capacity = 10;
    cache.count = 0;
    pthread_mutex_init(&cache.lock, NULL);
    
    bool result = db2_add_prepared_statement(&cache, NULL);
    TEST_ASSERT_FALSE(result);
    
    pthread_mutex_destroy(&cache.lock);
    free(cache.names);
}

// Test: Add statement to empty cache
void test_add_prepared_statement_empty_cache(void) {
    PreparedStatementCache cache = {0};
    cache.names = calloc(10, sizeof(char*));
    cache.capacity = 10;
    cache.count = 0;
    pthread_mutex_init(&cache.lock, NULL);
    
    bool result = db2_add_prepared_statement(&cache, "stmt_1");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, cache.count);
    TEST_ASSERT_NOT_NULL(cache.names[0]);
    TEST_ASSERT_EQUAL_STRING("stmt_1", cache.names[0]);
    
    // Cleanup
    free(cache.names[0]);
    pthread_mutex_destroy(&cache.lock);
    free(cache.names);
}

// Test: Add duplicate statement (should return true, already exists)
void test_add_prepared_statement_duplicate(void) {
    PreparedStatementCache cache = {0};
    cache.names = calloc(10, sizeof(char*));
    cache.capacity = 10;
    cache.count = 1;
    cache.names[0] = strdup("existing_stmt");
    pthread_mutex_init(&cache.lock, NULL);
    
    bool result = db2_add_prepared_statement(&cache, "existing_stmt");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, cache.count); // Count should not increase
    
    // Cleanup
    free(cache.names[0]);
    pthread_mutex_destroy(&cache.lock);
    free(cache.names);
}

// Test: Add multiple statements
void test_add_prepared_statement_multiple(void) {
    PreparedStatementCache cache = {0};
    cache.names = calloc(10, sizeof(char*));
    cache.capacity = 10;
    cache.count = 0;
    pthread_mutex_init(&cache.lock, NULL);
    
    // Add first statement
    bool result1 = db2_add_prepared_statement(&cache, "stmt_1");
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_EQUAL(1, cache.count);
    
    // Add second statement
    bool result2 = db2_add_prepared_statement(&cache, "stmt_2");
    TEST_ASSERT_TRUE(result2);
    TEST_ASSERT_EQUAL(2, cache.count);
    
    // Add third statement
    bool result3 = db2_add_prepared_statement(&cache, "stmt_3");
    TEST_ASSERT_TRUE(result3);
    TEST_ASSERT_EQUAL(3, cache.count);
    
    // Verify all names
    TEST_ASSERT_EQUAL_STRING("stmt_1", cache.names[0]);
    TEST_ASSERT_EQUAL_STRING("stmt_2", cache.names[1]);
    TEST_ASSERT_EQUAL_STRING("stmt_3", cache.names[2]);
    
    // Cleanup
    for (size_t i = 0; i < cache.count; i++) {
        free(cache.names[i]);
    }
    pthread_mutex_destroy(&cache.lock);
    free(cache.names);
}

// Test: Fill cache to capacity and trigger expansion
void test_add_prepared_statement_capacity_expansion(void) {
    PreparedStatementCache cache = {0};
    cache.names = calloc(2, sizeof(char*));  // Small initial capacity
    cache.capacity = 2;
    cache.count = 0;
    pthread_mutex_init(&cache.lock, NULL);
    
    // Fill to capacity
    TEST_ASSERT_TRUE(db2_add_prepared_statement(&cache, "stmt_1"));
    TEST_ASSERT_EQUAL(1, cache.count);
    TEST_ASSERT_EQUAL(2, cache.capacity);
    
    TEST_ASSERT_TRUE(db2_add_prepared_statement(&cache, "stmt_2"));
    TEST_ASSERT_EQUAL(2, cache.count);
    TEST_ASSERT_EQUAL(2, cache.capacity);
    
    // This should trigger capacity expansion (2 * 2 = 4)
    TEST_ASSERT_TRUE(db2_add_prepared_statement(&cache, "stmt_3"));
    TEST_ASSERT_EQUAL(3, cache.count);
    TEST_ASSERT_EQUAL(4, cache.capacity);  // Should have expanded
    
    // Verify all names are still accessible
    TEST_ASSERT_EQUAL_STRING("stmt_1", cache.names[0]);
    TEST_ASSERT_EQUAL_STRING("stmt_2", cache.names[1]);
    TEST_ASSERT_EQUAL_STRING("stmt_3", cache.names[2]);
    
    // Cleanup
    for (size_t i = 0; i < cache.count; i++) {
        free(cache.names[i]);
    }
    pthread_mutex_destroy(&cache.lock);
    free(cache.names);
}

// Test: Add statement with long name
void test_add_prepared_statement_long_name(void) {
    PreparedStatementCache cache = {0};
    cache.names = calloc(10, sizeof(char*));
    cache.capacity = 10;
    cache.count = 0;
    pthread_mutex_init(&cache.lock, NULL);
    
    char long_name[256];
    memset(long_name, 'A', 255);
    long_name[255] = '\0';
    
    bool result = db2_add_prepared_statement(&cache, long_name);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, cache.count);
    TEST_ASSERT_EQUAL_STRING(long_name, cache.names[0]);
    
    // Cleanup
    free(cache.names[0]);
    pthread_mutex_destroy(&cache.lock);
    free(cache.names);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_add_prepared_statement_null_cache);
    RUN_TEST(test_add_prepared_statement_null_name);
    RUN_TEST(test_add_prepared_statement_empty_cache);
    RUN_TEST(test_add_prepared_statement_duplicate);
    RUN_TEST(test_add_prepared_statement_multiple);
    RUN_TEST(test_add_prepared_statement_capacity_expansion);
    RUN_TEST(test_add_prepared_statement_long_name);
    
    return UNITY_END();
}