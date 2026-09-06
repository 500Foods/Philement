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

void test_execute_migration_apply_empty_cache(void);
void test_execute_migration_apply_apply_failure(void);
void test_execute_migration_apply_apply_unchanged(void);

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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_execute_migration_apply_empty_cache);
    RUN_TEST(test_execute_migration_apply_apply_failure);
    RUN_TEST(test_execute_migration_apply_apply_unchanged);
    return UNITY_END();
}
