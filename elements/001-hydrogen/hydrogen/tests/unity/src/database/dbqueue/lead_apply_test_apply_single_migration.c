/*
 * Unity Test File: lead_apply_test_apply_single_migration
 * This file contains unit tests for database_queue_apply_single_migration function
 * Focuses on error paths and edge cases to improve coverage
 */

// Enable mocks BEFORE any includes
#define USE_MOCK_SYSTEM
#define USE_MOCK_DATABASE_ENGINE

// Project includes
#include <src/hydrogen.h>
#include <tests/unity/mocks/mock_system.h>
#include <tests/unity/mocks/mock_database_engine.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database.h>
#include <src/database/database_cache.h>
#include <src/database/database_engine.h>

// Forward declarations for functions being tested
bool database_queue_apply_single_migration(DatabaseQueue* lead_queue, long long migration_id, const char* dqm_label);
bool parse_sql_statements(const char* sql, size_t sql_len, char*** statements, size_t* statement_count, 
                         size_t* statements_capacity, const char* delimiter, const char* dqm_label);

// Test function prototypes
void test_apply_single_migration_no_query_cache(void);
void test_apply_single_migration_entry_not_found(void);
void test_apply_single_migration_strdup_failure(void);
void test_apply_single_migration_parse_sql_failure(void);
void test_apply_single_migration_begin_transaction_failure(void);
void test_apply_single_migration_statement_calloc_failure(void);
void test_apply_single_migration_query_id_strdup_failure(void);
void test_apply_single_migration_no_persistent_connection(void);
void test_apply_single_migration_execute_failure(void);
void test_apply_single_migration_commit_failure(void);
void test_apply_single_migration_rollback_path(void);

// Helper function to create a mock lead queue for testing
static DatabaseQueue* create_mock_lead_queue(const char* db_name) {
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    if (!queue) return NULL;

    queue->database_name = strdup(db_name);
    queue->is_lead_queue = true;
    queue->queue_type = strdup("Lead");
    queue->latest_applied_migration = 0;
    queue->query_cache = NULL;
    
    // Create a mock connection handle
    queue->persistent_connection = calloc(1, sizeof(DatabaseHandle));

    return queue;
}

// Helper function to destroy mock queue
static void destroy_mock_lead_queue(DatabaseQueue* queue) {
    if (!queue) return;

    free(queue->database_name);
    free(queue->queue_type);
    if (queue->query_cache) {
        query_cache_destroy(queue->query_cache, "testdb");
    }
    if (queue->persistent_connection) {
        free(queue->persistent_connection);
    }
    free(queue);
}

void setUp(void) {
    // Reset all mocks before each test
    mock_system_reset_all();
    mock_database_engine_reset_all();
}

void tearDown(void) {
    // Clean up after each test
    mock_system_reset_all();
    mock_database_engine_reset_all();
}

// Test database_queue_apply_single_migration with no query cache
void test_apply_single_migration_no_query_cache(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    TEST_ASSERT_NOT_NULL(queue);
    
    // Ensure no query cache
    queue->query_cache = NULL;
    
    bool result = database_queue_apply_single_migration(queue, 1, "Test-01");
    TEST_ASSERT_FALSE(result);
    
    destroy_mock_lead_queue(queue);
}

// Test database_queue_apply_single_migration when entry not found in cache
void test_apply_single_migration_entry_not_found(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    TEST_ASSERT_NOT_NULL(queue);
    
    // Create an empty query cache (no entries)
    queue->query_cache = query_cache_create("testdb");
    TEST_ASSERT_NOT_NULL(queue->query_cache);
    
    bool result = database_queue_apply_single_migration(queue, 1, "Test-01");
    TEST_ASSERT_FALSE(result);
    
    query_cache_destroy(queue->query_cache, "testdb");
    queue->query_cache = NULL;
    destroy_mock_lead_queue(queue);
}

// Test database_queue_apply_single_migration when strdup fails for migration SQL
void test_apply_single_migration_strdup_failure(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    TEST_ASSERT_NOT_NULL(queue);
    
    // Create query cache and add an entry
    queue->query_cache = query_cache_create("testdb");
    TEST_ASSERT_NOT_NULL(queue->query_cache);
    
    QueryCacheEntry* entry = query_cache_entry_create(
        1, 1000, "SELECT 1;", "Test migration", "slow", 30, "testdb");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_TRUE(query_cache_add_entry(queue->query_cache, entry, "testdb"));
    
    // Make malloc fail (strdup uses malloc internally)
    // This will cause the strdup of migration_sql to fail
    mock_system_set_malloc_failure(1);
    
    bool result = database_queue_apply_single_migration(queue, 1, "Test-01");
    TEST_ASSERT_FALSE(result);
    
    query_cache_destroy(queue->query_cache, "testdb");
    queue->query_cache = NULL;
    destroy_mock_lead_queue(queue);
}

// Test database_queue_apply_single_migration when parse_sql_statements fails
void test_apply_single_migration_parse_sql_failure(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    TEST_ASSERT_NOT_NULL(queue);
    
    // Create query cache and add an entry with malformed SQL
    queue->query_cache = query_cache_create("testdb");
    TEST_ASSERT_NOT_NULL(queue->query_cache);
    
    // Add entry with empty SQL to trigger parse failure
    QueryCacheEntry* entry = query_cache_entry_create(
        1, 1000, "", "Empty SQL migration", "slow", 30, "testdb");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_TRUE(query_cache_add_entry(queue->query_cache, entry, "testdb"));
    
    bool result = database_queue_apply_single_migration(queue, 1, "Test-01");
    TEST_ASSERT_FALSE(result);
    
    query_cache_destroy(queue->query_cache, "testdb");
    queue->query_cache = NULL;
    destroy_mock_lead_queue(queue);
}

// Test database_queue_apply_single_migration when begin transaction fails
void test_apply_single_migration_begin_transaction_failure(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    TEST_ASSERT_NOT_NULL(queue);
    
    // Create query cache and add an entry
    queue->query_cache = query_cache_create("testdb");
    TEST_ASSERT_NOT_NULL(queue->query_cache);
    
    QueryCacheEntry* entry = query_cache_entry_create(
        1, 1000, "SELECT 1;", "Test migration", "slow", 30, "testdb");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_TRUE(query_cache_add_entry(queue->query_cache, entry, "testdb"));
    
    // Make begin transaction fail
    mock_database_engine_set_begin_result(false);
    
    bool result = database_queue_apply_single_migration(queue, 1, "Test-01");
    TEST_ASSERT_FALSE(result);
    
    query_cache_destroy(queue->query_cache, "testdb");
    queue->query_cache = NULL;
    destroy_mock_lead_queue(queue);
}

// Test database_queue_apply_single_migration when calloc fails for statement request
void test_apply_single_migration_statement_calloc_failure(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    TEST_ASSERT_NOT_NULL(queue);
    
    // Create query cache and add an entry
    queue->query_cache = query_cache_create("testdb");
    TEST_ASSERT_NOT_NULL(queue->query_cache);
    
    QueryCacheEntry* entry = query_cache_entry_create(
        1, 1000, "SELECT 1;", "Test migration", "slow", 30, "testdb");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_TRUE(query_cache_add_entry(queue->query_cache, entry, "testdb"));
    
    // Make begin transaction succeed
    mock_database_engine_set_begin_result(true);
    
    // Make calloc fail on first call (for stmt_request)
    mock_system_set_calloc_failure(1);
    
    // Make rollback succeed so we clean up properly
    mock_database_engine_set_rollback_result(true);
    
    bool result = database_queue_apply_single_migration(queue, 1, "Test-01");
    TEST_ASSERT_FALSE(result);
    
    query_cache_destroy(queue->query_cache, "testdb");
    queue->query_cache = NULL;
    destroy_mock_lead_queue(queue);
}

// Test database_queue_apply_single_migration when strdup fails for query_id
void test_apply_single_migration_query_id_strdup_failure(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    TEST_ASSERT_NOT_NULL(queue);
    
    // Create query cache and add an entry
    queue->query_cache = query_cache_create("testdb");
    TEST_ASSERT_NOT_NULL(queue->query_cache);
    
    QueryCacheEntry* entry = query_cache_entry_create(
        1, 1000, "SELECT 1;", "Test migration", "slow", 30, "testdb");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_TRUE(query_cache_add_entry(queue->query_cache, entry, "testdb"));
    
    // Make begin transaction succeed
    mock_database_engine_set_begin_result(true);
    
    // Make malloc fail on 2nd call (strdup uses malloc)
    // First malloc is for migration_sql, second is for query_id
    mock_system_set_malloc_failure(2);
    
    // Make rollback succeed
    mock_database_engine_set_rollback_result(true);
    
    bool result = database_queue_apply_single_migration(queue, 1, "Test-01");
    TEST_ASSERT_FALSE(result);
    
    query_cache_destroy(queue->query_cache, "testdb");
    queue->query_cache = NULL;
    destroy_mock_lead_queue(queue);
}

// Test database_queue_apply_single_migration with no persistent connection
void test_apply_single_migration_no_persistent_connection(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    TEST_ASSERT_NOT_NULL(queue);
    
    // Create query cache and add an entry
    queue->query_cache = query_cache_create("testdb");
    TEST_ASSERT_NOT_NULL(queue->query_cache);
    
    QueryCacheEntry* entry = query_cache_entry_create(
        1, 1000, "SELECT 1;", "Test migration", "slow", 30, "testdb");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_TRUE(query_cache_add_entry(queue->query_cache, entry, "testdb"));
    
    // Remove persistent connection
    free(queue->persistent_connection);
    queue->persistent_connection = NULL;
    
    // Make begin transaction succeed
    mock_database_engine_set_begin_result(true);
    
    // Make rollback succeed
    mock_database_engine_set_rollback_result(true);
    
    bool result = database_queue_apply_single_migration(queue, 1, "Test-01");
    TEST_ASSERT_FALSE(result);
    
    query_cache_destroy(queue->query_cache, "testdb");
    queue->query_cache = NULL;
    destroy_mock_lead_queue(queue);
}

// Test database_queue_apply_single_migration when statement execution fails
void test_apply_single_migration_execute_failure(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    TEST_ASSERT_NOT_NULL(queue);
    
    // Create query cache and add an entry
    queue->query_cache = query_cache_create("testdb");
    TEST_ASSERT_NOT_NULL(queue->query_cache);
    
    QueryCacheEntry* entry = query_cache_entry_create(
        1, 1000, "SELECT 1;", "Test migration", "slow", 30, "testdb");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_TRUE(query_cache_add_entry(queue->query_cache, entry, "testdb"));
    
    // Make begin transaction succeed
    mock_database_engine_set_begin_result(true);
    
    // Make statement execution fail
    mock_database_engine_set_execute_result(false);
    
    // Make rollback succeed
    mock_database_engine_set_rollback_result(true);
    
    bool result = database_queue_apply_single_migration(queue, 1, "Test-01");
    TEST_ASSERT_FALSE(result);
    
    query_cache_destroy(queue->query_cache, "testdb");
    queue->query_cache = NULL;
    destroy_mock_lead_queue(queue);
}

// Test database_queue_apply_single_migration when commit fails
void test_apply_single_migration_commit_failure(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    TEST_ASSERT_NOT_NULL(queue);
    
    // Create query cache and add an entry
    queue->query_cache = query_cache_create("testdb");
    TEST_ASSERT_NOT_NULL(queue->query_cache);
    
    QueryCacheEntry* entry = query_cache_entry_create(
        1, 1000, "SELECT 1;", "Test migration", "slow", 30, "testdb");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_TRUE(query_cache_add_entry(queue->query_cache, entry, "testdb"));
    
    // Make begin transaction succeed
    mock_database_engine_set_begin_result(true);
    
    // Make statement execution succeed
    mock_database_engine_set_execute_result(true);
    
    // Make commit fail
    mock_database_engine_set_commit_result(false);
    
    bool result = database_queue_apply_single_migration(queue, 1, "Test-01");
    TEST_ASSERT_FALSE(result);
    
    query_cache_destroy(queue->query_cache, "testdb");
    queue->query_cache = NULL;
    destroy_mock_lead_queue(queue);
}

// Test database_queue_apply_single_migration rollback path when rollback fails
void test_apply_single_migration_rollback_path(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    TEST_ASSERT_NOT_NULL(queue);
    
    // Create query cache and add an entry
    queue->query_cache = query_cache_create("testdb");
    TEST_ASSERT_NOT_NULL(queue->query_cache);
    
    QueryCacheEntry* entry = query_cache_entry_create(
        1, 1000, "SELECT 1;", "Test migration", "slow", 30, "testdb");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_TRUE(query_cache_add_entry(queue->query_cache, entry, "testdb"));
    
    // Make begin transaction succeed
    mock_database_engine_set_begin_result(true);
    
    // Make statement execution fail to trigger rollback
    mock_database_engine_set_execute_result(false);
    
    // Make rollback fail
    mock_database_engine_set_rollback_result(false);
    
    bool result = database_queue_apply_single_migration(queue, 1, "Test-01");
    TEST_ASSERT_FALSE(result);
    
    query_cache_destroy(queue->query_cache, "testdb");
    queue->query_cache = NULL;
    destroy_mock_lead_queue(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_apply_single_migration_no_query_cache);
    RUN_TEST(test_apply_single_migration_entry_not_found);
    RUN_TEST(test_apply_single_migration_strdup_failure);
    RUN_TEST(test_apply_single_migration_parse_sql_failure);
    RUN_TEST(test_apply_single_migration_begin_transaction_failure);
    RUN_TEST(test_apply_single_migration_statement_calloc_failure);
    RUN_TEST(test_apply_single_migration_query_id_strdup_failure);
    RUN_TEST(test_apply_single_migration_no_persistent_connection);
    RUN_TEST(test_apply_single_migration_execute_failure);
    RUN_TEST(test_apply_single_migration_commit_failure);
    RUN_TEST(test_apply_single_migration_rollback_path);

    return UNITY_END();
}