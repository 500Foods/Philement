/*
 * Unity Test File: lead_apply_test_execute_migration_apply
 * This file contains unit tests for database_queue_lead_execute_migration_apply
 */

#define USE_MOCK_SYSTEM
#define USE_MOCK_DATABASE_ENGINE

#include <src/hydrogen.h>
#include <tests/unity/mocks/mock_system.h>
#include <tests/unity/mocks/mock_database_engine.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/database_cache.h>
#include <src/database/database_engine.h>

bool database_queue_lead_execute_migration_apply(DatabaseQueue* lead_queue);

// Test function prototypes
void test_execute_migration_apply_empty_cache(void);
void test_execute_migration_apply_apply_failure(void);
void test_execute_migration_apply_apply_unchanged(void);
void test_execute_migration_apply_success_single(void);
void test_execute_migration_apply_success_multiple(void);
void test_execute_migration_apply_rollback_on_execute_failure(void);

static DatabaseQueue* create_mock_lead_queue(const char* db_name) {
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    if (!queue) return NULL;

    queue->database_name = strdup(db_name);
    queue->is_lead_queue = true;
    queue->queue_type = strdup("Lead");
    queue->latest_applied_migration = 0;
    queue->bootstrap_query = strdup("SELECT 1");
    queue->persistent_connection = calloc(1, sizeof(DatabaseHandle));
    if (queue->persistent_connection) {
        queue->persistent_connection->engine_type = DB_ENGINE_POSTGRESQL;
    }
    pthread_mutex_init(&queue->bootstrap_lock, NULL);
    pthread_cond_init(&queue->bootstrap_cond, NULL);
    return queue;
}

static void destroy_mock_lead_queue(DatabaseQueue* queue) {
    if (!queue) return;
    free(queue->database_name);
    free(queue->queue_type);
    free(queue->bootstrap_query);
    if (queue->query_cache) {
        query_cache_destroy(queue->query_cache, "testdb");
    }
    free(queue->persistent_connection);
    pthread_mutex_destroy(&queue->bootstrap_lock);
    pthread_cond_destroy(&queue->bootstrap_cond);
    free(queue);
}

void setUp(void) {
    mock_system_reset_all();
    mock_database_engine_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
    mock_database_engine_reset_all();
}

void test_execute_migration_apply_empty_cache(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    TEST_ASSERT_NOT_NULL(queue);
    queue->query_cache = query_cache_create("testdb");
    TEST_ASSERT_NOT_NULL(queue->query_cache);
    mock_database_engine_set_execute_result(true);
    TEST_ASSERT_TRUE(database_queue_lead_execute_migration_apply(queue));
    destroy_mock_lead_queue(queue);
}

void test_execute_migration_apply_apply_failure(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    QueryCacheEntry* entry;
    TEST_ASSERT_NOT_NULL(queue);
    queue->query_cache = query_cache_create("testdb");
    TEST_ASSERT_NOT_NULL(queue->query_cache);
    entry = query_cache_entry_create(
        1, 1000, "SELECT 1;", "Test migration", "slow", 30, "testdb");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_TRUE(query_cache_add_entry(queue->query_cache, entry, "testdb"));
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_begin_result(false);
    TEST_ASSERT_FALSE(database_queue_lead_execute_migration_apply(queue));
    destroy_mock_lead_queue(queue);
}

void test_execute_migration_apply_apply_unchanged(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    QueryCacheEntry* entry;
    TEST_ASSERT_NOT_NULL(queue);
    queue->query_cache = query_cache_create("testdb");
    TEST_ASSERT_NOT_NULL(queue->query_cache);
    entry = query_cache_entry_create(
        1, 1000, "SELECT 1;", "Test migration", "slow", 30, "testdb");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_TRUE(query_cache_add_entry(queue->query_cache, entry, "testdb"));
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_commit_result(true);
    TEST_ASSERT_FALSE(database_queue_lead_execute_migration_apply(queue));
    destroy_mock_lead_queue(queue);
}

// Test database_queue_lead_execute_migration_apply with a single migration
// that successfully applies and updates latest_applied_migration.
// The bootstrap query mock returns JSON with a type 1003 (applied) entry
// so that after apply_single_migration succeeds, the second bootstrap query
// updates latest_applied_migration from 0 to 1, preventing a stall.
void test_execute_migration_apply_success_single(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    TEST_ASSERT_NOT_NULL(queue);

    queue->query_cache = query_cache_create("testdb");
    TEST_ASSERT_NOT_NULL(queue->query_cache);

    // Add type 1000 (loaded forward migration) entry to QTC
    QueryCacheEntry* entry = query_cache_entry_create(
        1, 1000, "SELECT 1;", "Test migration", "slow", 30, "testdb");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_TRUE(query_cache_add_entry(queue->query_cache, entry, "testdb"));

    // Mock bootstrap query result: include a type 1003 (applied) entry
    // so latest_applied_migration gets set to 1 after the second bootstrap
    const char* bootstrap_json =
        "[{\"ref\":1,\"type\":1003,\"query\":\"SELECT 1;\",\"name\":\"Test migration\",\"queue\":0,\"timeout\":30}]";
    mock_database_engine_set_execute_json_data(bootstrap_json);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_commit_result(true);

    queue->latest_applied_migration = 0;

    bool result = database_queue_lead_execute_migration_apply(queue);
    TEST_ASSERT_TRUE(result);

    destroy_mock_lead_queue(queue);
}

// Test database_queue_lead_execute_migration_apply with multiple migrations
void test_execute_migration_apply_success_multiple(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    TEST_ASSERT_NOT_NULL(queue);

    queue->query_cache = query_cache_create("testdb");
    TEST_ASSERT_NOT_NULL(queue->query_cache);

    // Add two migration entries to QTC (types 1000 = loaded forward)
    QueryCacheEntry* entry1 = query_cache_entry_create(
        1, 1000, "SELECT 1;", "Migration 1", "slow", 30, "testdb");
    TEST_ASSERT_NOT_NULL(entry1);
    TEST_ASSERT_TRUE(query_cache_add_entry(queue->query_cache, entry1, "testdb"));

    QueryCacheEntry* entry2 = query_cache_entry_create(
        2, 1000, "SELECT 2;", "Migration 2", "slow", 30, "testdb");
    TEST_ASSERT_NOT_NULL(entry2);
    TEST_ASSERT_TRUE(query_cache_add_entry(queue->query_cache, entry2, "testdb"));

    // Mock bootstrap query result with both migrations marked as applied (1003)
    const char* bootstrap_json =
        "[{\"ref\":1,\"type\":1003,\"query\":\"SELECT 1;\",\"name\":\"M1\",\"queue\":0,\"timeout\":30},"
        "{\"ref\":2,\"type\":1003,\"query\":\"SELECT 2;\",\"name\":\"M2\",\"queue\":0,\"timeout\":30}]";
    mock_database_engine_set_execute_json_data(bootstrap_json);
    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_commit_result(true);

    queue->latest_applied_migration = 0;

    bool result = database_queue_lead_execute_migration_apply(queue);
    TEST_ASSERT_TRUE(result);

    destroy_mock_lead_queue(queue);
}

// Test database_queue_lead_execute_migration_apply when execute fails
// and rollback succeeds
void test_execute_migration_apply_rollback_on_execute_failure(void) {
    DatabaseQueue* queue = create_mock_lead_queue("testdb");
    TEST_ASSERT_NOT_NULL(queue);

    queue->query_cache = query_cache_create("testdb");
    TEST_ASSERT_NOT_NULL(queue->query_cache);

    QueryCacheEntry* entry = query_cache_entry_create(
        1, 1000, "SELECT 1;", "Test migration", "slow", 30, "testdb");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_TRUE(query_cache_add_entry(queue->query_cache, entry, "testdb"));

    mock_database_engine_set_execute_result(true);
    mock_database_engine_set_execute_json_data("[]");
    mock_database_engine_set_begin_result(true);
    mock_database_engine_set_execute_result(false);
    mock_database_engine_set_rollback_result(true);

    queue->latest_applied_migration = 0;

    bool result = database_queue_lead_execute_migration_apply(queue);
    TEST_ASSERT_FALSE(result);

    destroy_mock_lead_queue(queue);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_execute_migration_apply_empty_cache);
    RUN_TEST(test_execute_migration_apply_apply_failure);
    RUN_TEST(test_execute_migration_apply_apply_unchanged);
    RUN_TEST(test_execute_migration_apply_success_single);
    RUN_TEST(test_execute_migration_apply_success_multiple);
    RUN_TEST(test_execute_migration_apply_rollback_on_execute_failure);
    return UNITY_END();
}
