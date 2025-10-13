/*
 * Unity Test File: prepared_test_cache_operations
 * Tests for SQLite prepared statement cache add/remove operations
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/database/database.h>
#include <src/database/sqlite/types.h>
#include <src/database/sqlite/prepared.h>
#include <src/database/sqlite/connection.h>

// Forward declarations
bool sqlite_add_prepared_statement(PreparedStatementCache* cache, const char* name);
bool sqlite_remove_prepared_statement(PreparedStatementCache* cache, const char* name);
PreparedStatementCache* sqlite_create_prepared_statement_cache(void);
void sqlite_destroy_prepared_statement_cache(PreparedStatementCache* cache);

// Function prototypes
void test_add_prepared_statement_duplicate(void);
void test_add_prepared_statement_capacity_expansion(void);
void test_remove_prepared_statement_from_middle(void);
void test_remove_prepared_statement_first(void);
void test_remove_prepared_statement_last(void);
void test_add_prepared_statement_long_name(void);
void test_add_remove_multiple_cycles(void);

void setUp(void) {
    // Setup before each test
}

void tearDown(void) {
    // Cleanup after each test
}

// Test: Add statement with duplicate detection
void test_add_prepared_statement_duplicate(void) {
    PreparedStatementCache* cache = sqlite_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);
    
    // Add first statement
    TEST_ASSERT_TRUE(sqlite_add_prepared_statement(cache, "test_stmt"));
    TEST_ASSERT_EQUAL(1, cache->count);
    
    // Try to add same statement again - should return true but not increment count
    TEST_ASSERT_TRUE(sqlite_add_prepared_statement(cache, "test_stmt"));
    TEST_ASSERT_EQUAL(1, cache->count);
    
    // Cleanup
    sqlite_destroy_prepared_statement_cache(cache);
}

// Test: Add multiple statements to trigger capacity expansion
void test_add_prepared_statement_capacity_expansion(void) {
    PreparedStatementCache* cache = sqlite_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);
    
    size_t initial_capacity = cache->capacity;
    TEST_ASSERT_TRUE(initial_capacity > 0); // Has some default capacity
    
    // Fill to capacity
    for (size_t i = 0; i < initial_capacity; i++) {
        char name[32];
        snprintf(name, sizeof(name), "stmt_%zu", i);
        TEST_ASSERT_TRUE(sqlite_add_prepared_statement(cache, name));
    }
    TEST_ASSERT_EQUAL(initial_capacity, cache->count);
    TEST_ASSERT_EQUAL(initial_capacity, cache->capacity);
    
    // This should trigger capacity expansion (10 * 2 = 20)
    TEST_ASSERT_TRUE(sqlite_add_prepared_statement(cache, "stmt_trigger_expansion"));
    TEST_ASSERT_EQUAL(initial_capacity + 1, cache->count);
    TEST_ASSERT_EQUAL(initial_capacity * 2, cache->capacity); // Should have doubled
    
    // Verify all names are still accessible
    for (size_t i = 0; i < initial_capacity; i++) {
        TEST_ASSERT_NOT_NULL(cache->names[i]);
    }
    TEST_ASSERT_NOT_NULL(cache->names[initial_capacity]);
    TEST_ASSERT_EQUAL_STRING("stmt_trigger_expansion", cache->names[initial_capacity]);
    
    // Cleanup
    sqlite_destroy_prepared_statement_cache(cache);
}

// Test: Remove statement from middle of array (tests element shifting)
void test_remove_prepared_statement_from_middle(void) {
    PreparedStatementCache* cache = sqlite_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);
    
    // Add three statements
    TEST_ASSERT_TRUE(sqlite_add_prepared_statement(cache, "stmt_1"));
    TEST_ASSERT_TRUE(sqlite_add_prepared_statement(cache, "stmt_2"));
    TEST_ASSERT_TRUE(sqlite_add_prepared_statement(cache, "stmt_3"));
    TEST_ASSERT_EQUAL(3, cache->count);
    
    // Remove middle statement (tests element shifting)
    TEST_ASSERT_TRUE(sqlite_remove_prepared_statement(cache, "stmt_2"));
    TEST_ASSERT_EQUAL(2, cache->count);
    
    // Verify remaining statements are correctly shifted
    TEST_ASSERT_EQUAL_STRING("stmt_1", cache->names[0]);
    TEST_ASSERT_EQUAL_STRING("stmt_3", cache->names[1]);
    
    // Cleanup
    sqlite_destroy_prepared_statement_cache(cache);
}

// Test: Remove first statement
void test_remove_prepared_statement_first(void) {
    PreparedStatementCache* cache = sqlite_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);
    
    // Add statements
    TEST_ASSERT_TRUE(sqlite_add_prepared_statement(cache, "stmt_1"));
    TEST_ASSERT_TRUE(sqlite_add_prepared_statement(cache, "stmt_2"));
    TEST_ASSERT_TRUE(sqlite_add_prepared_statement(cache, "stmt_3"));
    TEST_ASSERT_EQUAL(3, cache->count);
    
    // Remove first statement
    TEST_ASSERT_TRUE(sqlite_remove_prepared_statement(cache, "stmt_1"));
    TEST_ASSERT_EQUAL(2, cache->count);
    
    // Verify remaining statements
    TEST_ASSERT_EQUAL_STRING("stmt_2", cache->names[0]);
    TEST_ASSERT_EQUAL_STRING("stmt_3", cache->names[1]);
    
    // Cleanup
    sqlite_destroy_prepared_statement_cache(cache);
}

// Test: Remove last statement
void test_remove_prepared_statement_last(void) {
    PreparedStatementCache* cache = sqlite_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);
    
    // Add statements
    TEST_ASSERT_TRUE(sqlite_add_prepared_statement(cache, "stmt_1"));
    TEST_ASSERT_TRUE(sqlite_add_prepared_statement(cache, "stmt_2"));
    TEST_ASSERT_TRUE(sqlite_add_prepared_statement(cache, "stmt_3"));
    TEST_ASSERT_EQUAL(3, cache->count);
    
    // Remove last statement
    TEST_ASSERT_TRUE(sqlite_remove_prepared_statement(cache, "stmt_3"));
    TEST_ASSERT_EQUAL(2, cache->count);
    
    // Verify remaining statements
    TEST_ASSERT_EQUAL_STRING("stmt_1", cache->names[0]);
    TEST_ASSERT_EQUAL_STRING("stmt_2", cache->names[1]);
    
    // Cleanup
    sqlite_destroy_prepared_statement_cache(cache);
}

// Test: Add statement with very long name
void test_add_prepared_statement_long_name(void) {
    PreparedStatementCache* cache = sqlite_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);
    
    char long_name[256];
    memset(long_name, 'A', 255);
    long_name[255] = '\0';
    
    TEST_ASSERT_TRUE(sqlite_add_prepared_statement(cache, long_name));
    TEST_ASSERT_EQUAL(1, cache->count);
    TEST_ASSERT_EQUAL_STRING(long_name, cache->names[0]);
    
    // Cleanup
    sqlite_destroy_prepared_statement_cache(cache);
}

// Test: Add and remove multiple times
void test_add_remove_multiple_cycles(void) {
    PreparedStatementCache* cache = sqlite_create_prepared_statement_cache();
    TEST_ASSERT_NOT_NULL(cache);
    
    // Cycle 1: Add and remove
    TEST_ASSERT_TRUE(sqlite_add_prepared_statement(cache, "stmt_1"));
    TEST_ASSERT_EQUAL(1, cache->count);
    TEST_ASSERT_TRUE(sqlite_remove_prepared_statement(cache, "stmt_1"));
    TEST_ASSERT_EQUAL(0, cache->count);
    
    // Cycle 2: Add different statement
    TEST_ASSERT_TRUE(sqlite_add_prepared_statement(cache, "stmt_2"));
    TEST_ASSERT_EQUAL(1, cache->count);
    TEST_ASSERT_EQUAL_STRING("stmt_2", cache->names[0]);
    
    // Cycle 3: Add multiple, remove one, add another
    TEST_ASSERT_TRUE(sqlite_add_prepared_statement(cache, "stmt_3"));
    TEST_ASSERT_TRUE(sqlite_add_prepared_statement(cache, "stmt_4"));
    TEST_ASSERT_EQUAL(3, cache->count);
    TEST_ASSERT_TRUE(sqlite_remove_prepared_statement(cache, "stmt_3"));
    TEST_ASSERT_EQUAL(2, cache->count);
    TEST_ASSERT_TRUE(sqlite_add_prepared_statement(cache, "stmt_5"));
    TEST_ASSERT_EQUAL(3, cache->count);
    
    // Cleanup
    sqlite_destroy_prepared_statement_cache(cache);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_add_prepared_statement_duplicate);
    RUN_TEST(test_add_prepared_statement_capacity_expansion);
    RUN_TEST(test_remove_prepared_statement_from_middle);
    RUN_TEST(test_remove_prepared_statement_first);
    RUN_TEST(test_remove_prepared_statement_last);
    RUN_TEST(test_add_prepared_statement_long_name);
    RUN_TEST(test_add_remove_multiple_cycles);
    
    return UNITY_END();
}