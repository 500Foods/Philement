/*
 * Unity Test File: database_bootstrap_test_execute_bootstrap_query
 * This file contains unit tests for database_queue_execute_bootstrap_query function
 * from src/database/database_bootstrap.c
 */

#define USE_MOCK_SYSTEM
#define USE_MOCK_DATABASE_ENGINE

#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_database_engine.h>

#include "unity.h"

// Include source headers after mocks
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/database_bootstrap.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database_cache.h>
#include <src/queue/queue.h>
#include <src/config/config_defaults.h>

struct QueryTableCache;
struct QueryCacheEntry;

// Forward declarations needed for testing
extern char* database_queue_generate_label(DatabaseQueue* db_queue);
extern MutexResult mutex_lock(pthread_mutex_t* mutex, const char* label);
extern MutexResult mutex_unlock(pthread_mutex_t* mutex);
extern struct QueryTableCache* query_cache_create(const char* dqm_label);
extern bool query_cache_add_entry(struct QueryTableCache* cache, struct QueryCacheEntry* entry, const char* dqm_label);
extern struct QueryCacheEntry* query_cache_entry_create(int query_ref, int query_type, const char* sql_template, const char* description, const char* queue_type, int timeout_seconds, const char* dqm_label);
extern void query_cache_entry_destroy(struct QueryCacheEntry* entry);
extern size_t query_cache_get_entry_count(struct QueryTableCache* cache);
extern void query_cache_clear(struct QueryTableCache* cache, const char* dqm_label);
extern struct QueryCacheEntry* query_cache_lookup(struct QueryTableCache* cache, int query_ref, const char* dqm_label);
extern void database_engine_cleanup_result(QueryResult* result);

// Test function prototypes
void test_null_db_queue(void);
void test_non_lead_queue(void);
void test_lead_queue_no_connection(void);
void test_request_allocation_failure(void);
void test_query_id_allocation_failure(void);
void test_sql_template_allocation_failure(void);
void test_parameters_json_allocation_failure(void);
void test_query_execution_failure(void);
void test_successful_execution_no_qtc(void);
void test_successful_execution_with_qtc(void);
void test_qtc_add_entry_failure(void);
void test_migration_tracking_available(void);
void test_migration_tracking_installed(void);
void test_migration_tracking_mixed(void);
void test_bootstrap_completion_signaling(void);
void test_empty_database_detection(void);

void setUp(void) {
    mock_system_reset_all();
    mock_database_engine_reset_all();
    
    // Ensure queue system is initialized if needed
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    mock_system_reset_all();
    mock_database_engine_reset_all();

    // Small delay for cleanup
    usleep(10000);  // 10ms
}

// Test: NULL db_queue parameter handling (line 33)
void test_null_db_queue(void) {
    database_queue_execute_bootstrap_query(NULL);
    // Should not crash - success if no segfault
}

// Test: Non-lead queue handling (line 33)
void test_non_lead_queue(void) {
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    if (!queue) return;
    queue->is_lead_queue = false;
    queue->database_name = strdup("test_non_lead");

    database_queue_execute_bootstrap_query(queue);

    free(queue->database_name);
    free(queue);
    // Should return early without processing
}

// Test: Lead queue with no connection (lines 92-95)
void test_lead_queue_no_connection(void) {
    database_subsystem_init();
    
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    if (!queue) return;
    queue->is_lead_queue = true;
    queue->database_name = strdup("test_no_conn");
    queue->persistent_connection = NULL;
    queue->is_connected = false;

    // Initialize locks
    // cppcheck-suppress nullPointerOutOfMemory
    pthread_mutex_init(&queue->connection_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    pthread_mutex_init(&queue->bootstrap_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    pthread_cond_init(&queue->bootstrap_cond, NULL);

    database_queue_execute_bootstrap_query(queue);

    // Without a connection, bootstrap_completed won't be set (it's only set inside the connection block)
    // cppcheck-suppress nullPointerOutOfMemory
    TEST_ASSERT_FALSE(queue->bootstrap_completed);

    free(queue->database_name);
    pthread_mutex_destroy(&queue->connection_lock);
    pthread_mutex_destroy(&queue->bootstrap_lock);
    pthread_cond_destroy(&queue->bootstrap_cond);
    free(queue);
    
    database_subsystem_shutdown();
}

// Test: Request allocation failure (lines 46-50)
void test_request_allocation_failure(void) {
    database_subsystem_init();
    
    mock_system_set_calloc_failure(1);

    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    if (!queue) return; // Safety check
    queue->is_lead_queue = true;
    queue->database_name = strdup("test_alloc_fail");
    queue->bootstrap_query = strdup("SELECT 1");

    // Initialize locks
    // cppcheck-suppress nullPointerOutOfMemory
    pthread_mutex_init(&queue->connection_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    pthread_mutex_init(&queue->bootstrap_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    pthread_cond_init(&queue->bootstrap_cond, NULL);

    // Mock connection
    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    if (!conn) {
        free(queue->database_name);
        free(queue->bootstrap_query);
        free(queue);
        return;
    }
    // cppcheck-suppress nullPointerOutOfMemory
    conn->engine_type = DB_ENGINE_POSTGRESQL;
    // cppcheck-suppress nullPointerOutOfMemory
    conn->designator = strdup("mock_conn");
    pthread_mutex_init(&conn->connection_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    queue->persistent_connection = conn;
    // cppcheck-suppress nullPointerOutOfMemory
    queue->is_connected = true;

    mock_database_engine_set_execute_result(true);

    database_queue_execute_bootstrap_query(queue);

    // Should handle allocation failure gracefully
    // cppcheck-suppress nullPointerOutOfMemory
    TEST_ASSERT_TRUE(queue->bootstrap_completed);

    // Cleanup
    free(queue->database_name);
    free(queue->bootstrap_query);
    // cppcheck-suppress nullPointerOutOfMemory
    if (conn->designator) free(conn->designator);
    pthread_mutex_destroy(&conn->connection_lock);
    free(conn);
    pthread_mutex_destroy(&queue->connection_lock);
    pthread_mutex_destroy(&queue->bootstrap_lock);
    pthread_cond_destroy(&queue->bootstrap_cond);
    free(queue);

    mock_system_set_calloc_failure(0);
    database_subsystem_shutdown();
}

// Test: query_id allocation failure (lines 55-60)
void test_query_id_allocation_failure(void) {
    database_subsystem_init();
    
    // Allocate and setup queue BEFORE setting malloc failure
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    if (!queue) return;
    // cppcheck-suppress nullPointerOutOfMemory
    queue->is_lead_queue = true;
    // cppcheck-suppress nullPointerOutOfMemory
    queue->database_name = strdup("test_query_id_fail");
    // cppcheck-suppress nullPointerOutOfMemory
    queue->bootstrap_query = strdup("SELECT 1");  // Provide bootstrap query
    
    // Initialize locks and connection (similar to above)
    // cppcheck-suppress nullPointerOutOfMemory
    pthread_mutex_init(&queue->connection_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    pthread_mutex_init(&queue->bootstrap_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    pthread_cond_init(&queue->bootstrap_cond, NULL);
    
    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    if (!conn) {
        free(queue->database_name);
        free(queue->bootstrap_query);
        pthread_mutex_destroy(&queue->connection_lock);
        pthread_mutex_destroy(&queue->bootstrap_lock);
        pthread_cond_destroy(&queue->bootstrap_cond);
        free(queue);
        database_subsystem_shutdown();
        return;
    }
    // cppcheck-suppress nullPointerOutOfMemory
    conn->engine_type = DB_ENGINE_POSTGRESQL;
    // cppcheck-suppress nullPointerOutOfMemory
    conn->designator = strdup("mock_conn");
    pthread_mutex_init(&conn->connection_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    queue->persistent_connection = conn;
    // cppcheck-suppress nullPointerOutOfMemory
    queue->is_connected = true;
    
    // NOW set malloc failure - this will fail the query_id strdup inside the function
    // Count: 1=request calloc, 2=dqm_label strdup, 3=query_id strdup
    // We want to fail query_id, so fail on call #3
    mock_system_set_malloc_failure(3);
    
    database_queue_execute_bootstrap_query(queue);
    
    // Bootstrap should NOT complete due to early allocation failure
    // (function returns before setting bootstrap_completed)
    // cppcheck-suppress nullPointerOutOfMemory
    TEST_ASSERT_FALSE(queue->bootstrap_completed);
    
    // Cleanup similar to above
    // cppcheck-suppress nullPointerOutOfMemory
    free(queue->database_name);
    // cppcheck-suppress nullPointerOutOfMemory
    free(queue->bootstrap_query);
    // cppcheck-suppress nullPointerOutOfMemory
    if (conn->designator) free(conn->designator);
    pthread_mutex_destroy(&conn->connection_lock);
    free(conn);
    pthread_mutex_destroy(&queue->connection_lock);
    pthread_mutex_destroy(&queue->bootstrap_lock);
    pthread_cond_destroy(&queue->bootstrap_cond);
    free(queue);
    
    mock_system_set_malloc_failure(0);
    database_subsystem_shutdown();
}

// Test: sql_template allocation failure (lines 63-69)
void test_sql_template_allocation_failure(void) {
    database_subsystem_init();
    
    // Allocate queue structures BEFORE setting malloc failure
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    if (!queue) {
        database_subsystem_shutdown();
        return;
    }
    // cppcheck-suppress nullPointerOutOfMemory
    queue->is_lead_queue = true;
    // cppcheck-suppress nullPointerOutOfMemory
    queue->database_name = strdup("test_sql_fail");
    // cppcheck-suppress nullPointerOutOfMemory
    queue->bootstrap_query = NULL;  // Will use default query which requires strdup
    
    pthread_mutex_init(&queue->connection_lock, NULL);
    pthread_mutex_init(&queue->bootstrap_lock, NULL);
    pthread_cond_init(&queue->bootstrap_cond, NULL);
    
    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    if (!conn) {
        free(queue->database_name);
        pthread_mutex_destroy(&queue->connection_lock);
        pthread_mutex_destroy(&queue->bootstrap_lock);
        pthread_cond_destroy(&queue->bootstrap_cond);
        free(queue);
        database_subsystem_shutdown();
        return;
    }
    // cppcheck-suppress nullPointerOutOfMemory
    conn->engine_type = DB_ENGINE_POSTGRESQL;
    // cppcheck-suppress nullPointerOutOfMemory
    conn->designator = strdup("mock_conn");
   pthread_mutex_init(&conn->connection_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    queue->persistent_connection = conn;
    // cppcheck-suppress nullPointerOutOfMemory
    queue->is_connected = true;
    
    // NOW set malloc failure for sql_template strdup
    // Count: 1=request calloc, 2=dqm_label strdup, 3=query_id strdup, 4=sql_template strdup
    mock_system_set_malloc_failure(4);
    
    database_queue_execute_bootstrap_query(queue);
    
    // Bootstrap should NOT complete due to early allocation failure
    // cppcheck-suppress nullPointerOutOfMemory
    TEST_ASSERT_FALSE(queue->bootstrap_completed);
    
    // Cleanup
    // cppcheck-suppress nullPointerOutOfMemory
    free(queue->database_name);
    // cppcheck-suppress nullPointerOutOfMemory
    if (conn->designator) free(conn->designator);
    pthread_mutex_destroy(&conn->connection_lock);
    free(conn);
    pthread_mutex_destroy(&queue->connection_lock);
    pthread_mutex_destroy(&queue->bootstrap_lock);
    pthread_cond_destroy(&queue->bootstrap_cond);
    free(queue);
    
    mock_system_set_malloc_failure(0);
    database_subsystem_shutdown();
}

// Test: parameters_json allocation failure (lines 72-79)
void test_parameters_json_allocation_failure(void) {
    database_subsystem_init();
    
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    // cppcheck-suppress nullPointerOutOfMemory
    queue->is_lead_queue = true;
    // cppcheck-suppress nullPointerOutOfMemory
    queue->database_name = strdup("test_params_fail");
    // cppcheck-suppress nullPointerOutOfMemory
    queue->bootstrap_query = strdup("SELECT 1");  // Ensure previous allocations can succeed
    
    pthread_mutex_init(&queue->connection_lock, NULL);
    pthread_mutex_init(&queue->bootstrap_lock, NULL);
    pthread_cond_init(&queue->bootstrap_cond, NULL);
    
    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    // cppcheck-suppress nullPointerOutOfMemory
    conn->engine_type = DB_ENGINE_POSTGRESQL;
    // cppcheck-suppress nullPointerOutOfMemory
    conn->designator = strdup("mock_conn");
    pthread_mutex_init(&conn->connection_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    queue->persistent_connection = conn;
    // cppcheck-suppress nullPointerOutOfMemory
    queue->is_connected = true;
    
    mock_system_set_malloc_failure(1);  // Fail strdup("{}") for parameters_json
    
    database_queue_execute_bootstrap_query(queue);
    
    // cppcheck-suppress nullPointerOutOfMemory
    TEST_ASSERT_TRUE(queue->bootstrap_completed);
    
    // Cleanup
    // cppcheck-suppress nullPointerOutOfMemory
    free(queue->database_name);
    // cppcheck-suppress nullPointerOutOfMemory
    free(queue->bootstrap_query);
    // cppcheck-suppress nullPointerOutOfMemory
    if (conn->designator) free(conn->designator);
    pthread_mutex_destroy(&conn->connection_lock);
    free(conn);
    pthread_mutex_destroy(&queue->connection_lock);
    pthread_mutex_destroy(&queue->bootstrap_lock);
    pthread_cond_destroy(&queue->bootstrap_cond);
    free(queue);
    
    mock_system_set_malloc_failure(0);
    database_subsystem_shutdown();
}

// Test: Query execution failure (lines 204-210)
void test_query_execution_failure(void) {
    database_subsystem_init();
    
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    // cppcheck-suppress nullPointerOutOfMemory
    queue->is_lead_queue = true;
    // cppcheck-suppress nullPointerOutOfMemory
    queue->database_name = strdup("test_exec_fail");
    // cppcheck-suppress nullPointerOutOfMemory
    queue->bootstrap_query = strdup("SELECT 1");  // Add missing bootstrap query
    
    pthread_mutex_init(&queue->connection_lock, NULL);
    pthread_mutex_init(&queue->bootstrap_lock, NULL);
    pthread_cond_init(&queue->bootstrap_cond, NULL);
    
    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    // cppcheck-suppress nullPointerOutOfMemory
    conn->engine_type = DB_ENGINE_POSTGRESQL;
    // cppcheck-suppress nullPointerOutOfMemory
    conn->designator = strdup("mock_conn");
    pthread_mutex_init(&conn->connection_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    queue->persistent_connection = conn;
    // cppcheck-suppress nullPointerOutOfMemory
    queue->is_connected = true;
    
    mock_database_engine_set_execute_result(false);
    mock_database_engine_set_execute_json_data("[]");  // Empty result
    
    database_queue_execute_bootstrap_query(queue);
    
    // Should detect empty database
    // cppcheck-suppress nullPointerOutOfMemory
    TEST_ASSERT_TRUE(queue->empty_database);
    // cppcheck-suppress nullPointerOutOfMemory
    TEST_ASSERT_EQUAL(0, queue->latest_available_migration);
    // cppcheck-suppress nullPointerOutOfMemory
    TEST_ASSERT_EQUAL(0, queue->latest_applied_migration);
    // Bootstrap always completes, even on query failure (per line 292 comment)
    // cppcheck-suppress nullPointerOutOfMemory
    TEST_ASSERT_TRUE(queue->bootstrap_completed);
    
    // Cleanup
    // cppcheck-suppress nullPointerOutOfMemory
    free(queue->database_name);
    // cppcheck-suppress nullPointerOutOfMemory
    free(queue->bootstrap_query);  // Free the bootstrap query
    // cppcheck-suppress nullPointerOutOfMemory
    if (conn->designator) free(conn->designator);
    pthread_mutex_destroy(&conn->connection_lock);
    free(conn);
    pthread_mutex_destroy(&queue->connection_lock);
    pthread_mutex_destroy(&queue->bootstrap_lock);
    pthread_cond_destroy(&queue->bootstrap_cond);
    free(queue);
    
    database_subsystem_shutdown();
}

// Test: Successful execution (bootstrap ALWAYS populates QTC now)
void test_successful_execution_no_qtc(void) {
    database_subsystem_init();
    
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    if (!queue) return;
    queue->is_lead_queue = true;
    queue->database_name = strdup("test_success_no_qtc");
    queue->bootstrap_query = strdup("SELECT 1");  // Add missing bootstrap query

    // cppcheck-suppress nullPointerOutOfMemory
    pthread_mutex_init(&queue->connection_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    pthread_mutex_init(&queue->bootstrap_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    pthread_cond_init(&queue->bootstrap_cond, NULL);

    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    if (!conn) {
        free(queue->database_name);
        free(queue->bootstrap_query);
        free(queue);
        return;
    }
    // cppcheck-suppress nullPointerOutOfMemory
    conn->engine_type = DB_ENGINE_POSTGRESQL;
    // cppcheck-suppress nullPointerOutOfMemory
    conn->designator = strdup("mock_conn");
    pthread_mutex_init(&conn->connection_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    queue->persistent_connection = conn;
    // cppcheck-suppress nullPointerOutOfMemory
    queue->is_connected = true;

    // JSON with migration data only
    const char* json_data = "["
        "{\"type\": 1000, \"ref\": 5},"
        "{\"type\": 1003, \"ref\": 3}"
    "]";
    mock_database_engine_set_execute_json_data(json_data);
    mock_database_engine_set_execute_result(true);

    database_queue_execute_bootstrap_query(queue);

    // Verify migration tracking
    // cppcheck-suppress nullPointerOutOfMemory
    TEST_ASSERT_EQUAL(5, queue->latest_loaded_migration);
    TEST_ASSERT_EQUAL(3, queue->latest_applied_migration);
    // cppcheck-suppress nullPointerOutOfMemory
    TEST_ASSERT_FALSE(queue->empty_database);
    // Bootstrap now ALWAYS populates QTC, but with no QTC data in JSON, cache will be created but empty
    TEST_ASSERT_NOT_NULL(queue->query_cache);
    TEST_ASSERT_EQUAL(0, query_cache_get_entry_count(queue->query_cache));
    // cppcheck-suppress nullPointerOutOfMemory
    TEST_ASSERT_TRUE(queue->bootstrap_completed);
    
    // Cleanup QTC
    query_cache_destroy(queue->query_cache, NULL);

    // Cleanup
    free(queue->database_name);
    free(queue->bootstrap_query);
    // cppcheck-suppress nullPointerOutOfMemory
    if (conn->designator) free(conn->designator);
    pthread_mutex_destroy(&conn->connection_lock);
    free(conn);
    pthread_mutex_destroy(&queue->connection_lock);
    pthread_mutex_destroy(&queue->bootstrap_lock);
    pthread_cond_destroy(&queue->bootstrap_cond);
    free(queue);
    
    database_subsystem_shutdown();
}

// Test: Successful execution with QTC population (lines 106-111, 142-166)
void test_successful_execution_with_qtc(void) {
    database_subsystem_init();
    
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    if (!queue) return;
    queue->is_lead_queue = true;
    queue->database_name = strdup("test_success_qtc");
    queue->bootstrap_query = strdup("SELECT 1");  // Add missing bootstrap query

    // cppcheck-suppress nullPointerOutOfMemory
    pthread_mutex_init(&queue->connection_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    pthread_mutex_init(&queue->bootstrap_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    pthread_cond_init(&queue->bootstrap_cond, NULL);

    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    if (!conn) {
        free(queue->database_name);
        free(queue->bootstrap_query);
        free(queue);
        return;
    }
    // cppcheck-suppress nullPointerOutOfMemory
    conn->engine_type = DB_ENGINE_POSTGRESQL;
    // cppcheck-suppress nullPointerOutOfMemory
    conn->designator = strdup("mock_conn");
    pthread_mutex_init(&conn->connection_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    queue->persistent_connection = conn;
    // cppcheck-suppress nullPointerOutOfMemory
    queue->is_connected = true;

    // JSON with QTC fields and migration
    const char* json_data = "["
        "{\"ref\": 1001, \"query\": \"SELECT * FROM users\", \"name\": \"Users Query\", \"queue\": 3, \"timeout\": 30, \"type\": 0},"
        "{\"type\": 1000, \"ref\": 10}"
    "]";
    mock_database_engine_set_execute_json_data(json_data);
    mock_database_engine_set_execute_result(true);

    database_queue_execute_bootstrap_query(queue);

    // Verify QTC created and populated
    TEST_ASSERT_NOT_NULL(queue->query_cache);
    TEST_ASSERT_EQUAL(1, query_cache_get_entry_count(queue->query_cache));

    // Verify migration
    TEST_ASSERT_EQUAL(10, queue->latest_loaded_migration);
    // cppcheck-suppress nullPointerOutOfMemory
    TEST_ASSERT_EQUAL(0, queue->latest_applied_migration);
    // cppcheck-suppress nullPointerOutOfMemory
    TEST_ASSERT_FALSE(queue->empty_database);
    // cppcheck-suppress nullPointerOutOfMemory
    TEST_ASSERT_TRUE(queue->bootstrap_completed);

    // Cleanup QTC
    query_cache_destroy(queue->query_cache, NULL);

    free(queue->database_name);
    free(queue->bootstrap_query);
    // cppcheck-suppress nullPointerOutOfMemory
    if (conn->designator) free(conn->designator);
    pthread_mutex_destroy(&conn->connection_lock);
    free(conn);
    pthread_mutex_destroy(&queue->connection_lock);
    pthread_mutex_destroy(&queue->bootstrap_lock);
    pthread_cond_destroy(&queue->bootstrap_cond);
    free(queue);
    
    database_subsystem_shutdown();
}

// Test: Migration tracking for available migrations (lines 180-184)
void test_migration_tracking_available(void) {
    database_subsystem_init();
    
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    if (!queue) return;
    queue->is_lead_queue = true;
    queue->database_name = strdup("test_mig_available");
    queue->bootstrap_query = strdup("SELECT 1");

    pthread_mutex_init(&queue->connection_lock, NULL);
    pthread_mutex_init(&queue->bootstrap_lock, NULL);
    pthread_cond_init(&queue->bootstrap_cond, NULL);

    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    if (!conn) {
        free(queue->database_name);
        free(queue);
        return;
    }
    // cppcheck-suppress nullPointerOutOfMemory
    conn->engine_type = DB_ENGINE_POSTGRESQL;
    // cppcheck-suppress nullPointerOutOfMemory
    conn->designator = strdup("mock_conn");
    pthread_mutex_init(&conn->connection_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    queue->persistent_connection = conn;
    // cppcheck-suppress nullPointerOutOfMemory
    queue->is_connected = true;

    const char* json_data = "["
        "{\"type\": 1000, \"ref\": 1},"
        "{\"type\": 1000, \"ref\": 5},"
        "{\"type\": 1000, \"ref\": 3}"
    "]";
    mock_database_engine_set_execute_json_data(json_data);
    mock_database_engine_set_execute_result(true);

    database_queue_execute_bootstrap_query(queue);

    // cppcheck-suppress nullPointerOutOfMemory
    TEST_ASSERT_EQUAL(5, queue->latest_loaded_migration);  // Max of 1,5,3
    // cppcheck-suppress nullPointerOutOfMemory
    TEST_ASSERT_EQUAL(0, queue->latest_applied_migration);

    free(queue->database_name);
    free(queue->bootstrap_query);
    // cppcheck-suppress nullPointerOutOfMemory
    if (conn->designator) free(conn->designator);
    pthread_mutex_destroy(&conn->connection_lock);
    free(conn);
    pthread_mutex_destroy(&queue->connection_lock);
    pthread_mutex_destroy(&queue->bootstrap_lock);
    pthread_cond_destroy(&queue->bootstrap_cond);
    free(queue);
    
    database_subsystem_shutdown();
}

// Test: Migration tracking for installed migrations (lines 185-189)
void test_migration_tracking_installed(void) {
    database_subsystem_init();
    
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    // cppcheck-suppress nullPointerOutOfMemory
    if (!queue) return;
    queue->is_lead_queue = true;
    queue->database_name = strdup("test_mig_installed");
    queue->bootstrap_query = strdup("SELECT 1");

    pthread_mutex_init(&queue->connection_lock, NULL);
    pthread_mutex_init(&queue->bootstrap_lock, NULL);
    pthread_cond_init(&queue->bootstrap_cond, NULL);

    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    // cppcheck-suppress nullPointerOutOfMemory
    if (!conn) {
        free(queue->database_name);
        free(queue);
        return;
    }
    // cppcheck-suppress nullPointerOutOfMemory
    conn->engine_type = DB_ENGINE_POSTGRESQL;
    // cppcheck-suppress nullPointerOutOfMemory
    conn->designator = strdup("mock_conn");

    pthread_mutex_init(&conn->connection_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    queue->persistent_connection = conn;
    // cppcheck-suppress nullPointerOutOfMemory
    queue->is_connected = true;

    const char* json_data = "["
        "{\"type\": 1003, \"ref\": 2},"
        "{\"type\": 1003, \"ref\": 7},"
        "{\"type\": 1003, \"ref\": 4}"
    "]";
    mock_database_engine_set_execute_json_data(json_data);
    mock_database_engine_set_execute_result(true);

    database_queue_execute_bootstrap_query(queue);

    TEST_ASSERT_EQUAL(0, queue->latest_loaded_migration);
    TEST_ASSERT_EQUAL(7, queue->latest_applied_migration);  // Max of 2,7,4

    free(queue->database_name);
    free(queue->bootstrap_query);
    // cppcheck-suppress nullPointerOutOfMemory
    if (conn->designator) free(conn->designator);
    pthread_mutex_destroy(&conn->connection_lock);
    free(conn);
    pthread_mutex_destroy(&queue->connection_lock);
    pthread_mutex_destroy(&queue->bootstrap_lock);
    pthread_cond_destroy(&queue->bootstrap_cond);
    free(queue);
    
    database_subsystem_shutdown();
}

// Test: Mixed migration types (lines 177-191)
void test_migration_tracking_mixed(void) {
    database_subsystem_init();
    
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    // cppcheck-suppress nullPointerOutOfMemory
    if (!queue) return;
    queue->is_lead_queue = true;
    queue->database_name = strdup("test_mig_mixed");
    queue->bootstrap_query = strdup("SELECT 1");

    pthread_mutex_init(&queue->connection_lock, NULL);
    pthread_mutex_init(&queue->bootstrap_lock, NULL);
    pthread_cond_init(&queue->bootstrap_cond, NULL);

    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    // cppcheck-suppress nullPointerOutOfMemory
    if (!conn) {
        free(queue->database_name);
        free(queue);
        return;
    }
    // cppcheck-suppress nullPointerOutOfMemory
    conn->engine_type = DB_ENGINE_POSTGRESQL;
    // cppcheck-suppress nullPointerOutOfMemory
    conn->designator = strdup("mock_conn");

    pthread_mutex_init(&conn->connection_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    queue->persistent_connection = conn;
    // cppcheck-suppress nullPointerOutOfMemory
    queue->is_connected = true;

    const char* json_data = "["
        "{\"type\": 1000, \"ref\": 6},"
        "{\"type\": 1003, \"ref\": 4},"
        "{\"type\": 1000, \"ref\": 8},"
        "{\"type\": 1003, \"ref\": 9}"
    "]";
    mock_database_engine_set_execute_json_data(json_data);
    mock_database_engine_set_execute_result(true);

    database_queue_execute_bootstrap_query(queue);

    TEST_ASSERT_EQUAL(8, queue->latest_loaded_migration);  // Max 1000: 6,8
    TEST_ASSERT_EQUAL(9, queue->latest_applied_migration);  // Max 1003: 4,9

    free(queue->database_name);
    free(queue->bootstrap_query);
    // cppcheck-suppress nullPointerOutOfMemory
    if (conn->designator) free(conn->designator);
    pthread_mutex_destroy(&conn->connection_lock);
    free(conn);
    pthread_mutex_destroy(&queue->connection_lock);
    pthread_mutex_destroy(&queue->bootstrap_lock);
    pthread_cond_destroy(&queue->bootstrap_cond);
    free(queue);
    
    database_subsystem_shutdown();
}

// Test: Bootstrap completion signaling (lines 226-232)
void test_bootstrap_completion_signaling(void) {
    database_subsystem_init();
    
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    // cppcheck-suppress nullPointerOutOfMemory
    if (!queue) return;
    queue->is_lead_queue = true;
    queue->database_name = strdup("test_bootstrap_signal");
    queue->bootstrap_query = strdup("SELECT 1");

    pthread_mutex_init(&queue->connection_lock, NULL);
    pthread_mutex_init(&queue->bootstrap_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    pthread_cond_init(&queue->bootstrap_cond, NULL);

    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    if (!conn) {
        free(queue->database_name);
        free(queue);
        return;
    }
    // cppcheck-suppress nullPointerOutOfMemory
    conn->engine_type = DB_ENGINE_POSTGRESQL;
    // cppcheck-suppress nullPointerOutOfMemory
    conn->designator = strdup("mock_conn");

    pthread_mutex_init(&conn->connection_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    queue->persistent_connection = conn;
    // cppcheck-suppress nullPointerOutOfMemory
    queue->is_connected = true;
    queue->bootstrap_completed = false;  // Initial state

    mock_database_engine_set_execute_result(true);

    database_queue_execute_bootstrap_query(queue);

    // cppcheck-suppress nullPointerOutOfMemory
    TEST_ASSERT_TRUE(queue->bootstrap_completed);

    free(queue->database_name);
    // cppcheck-suppress nullPointerOutOfMemory
    if (conn->designator) free(conn->designator);
    pthread_mutex_destroy(&conn->connection_lock);
    free(conn);
    // Functionally does nothing:
    pthread_mutex_destroy(&queue->bootstrap_lock);
    pthread_cond_destroy(&queue->bootstrap_cond);
    free(queue);
    
    database_subsystem_shutdown();
}

// Test: Empty database detection (lines 115, 204-210)
void test_empty_database_detection(void) {
    database_subsystem_init();
    
    DatabaseQueue* queue = calloc(1, sizeof(DatabaseQueue));
    // cppcheck-suppress nullPointerOutOfMemory
    if (!queue) return;
    queue->is_lead_queue = true;
    queue->database_name = strdup("test_empty_db");
    queue->bootstrap_query = strdup("SELECT 1");  // Add missing bootstrap query
    queue->empty_database = false;  // Initial

    pthread_mutex_init(&queue->connection_lock, NULL);
    pthread_mutex_init(&queue->bootstrap_lock, NULL);
    pthread_cond_init(&queue->bootstrap_cond, NULL);

    DatabaseHandle* conn = calloc(1, sizeof(DatabaseHandle));
    if (!conn) {
        free(queue->database_name);
        free(queue);
        return;
    }
    // cppcheck-suppress nullPointerOutOfMemory
    conn->engine_type = DB_ENGINE_POSTGRESQL;
    // cppcheck-suppress nullPointerOutOfMemory
    conn->designator = strdup("mock_conn");

    pthread_mutex_init(&conn->connection_lock, NULL);
    // cppcheck-suppress nullPointerOutOfMemory
    queue->persistent_connection = conn;
    // cppcheck-suppress nullPointerOutOfMemory
    queue->is_connected = true;

    // Empty result JSON
    mock_database_engine_set_execute_json_data("[]");
    mock_database_engine_set_execute_result(true);

    database_queue_execute_bootstrap_query(queue);

    TEST_ASSERT_TRUE(queue->empty_database);

    free(queue->database_name);
    free(queue->bootstrap_query);  // Free the bootstrap query
    // cppcheck-suppress nullPointerOutOfMemory
    if (conn->designator) free(conn->designator);
    pthread_mutex_destroy(&conn->connection_lock);
    free(conn);
    pthread_mutex_destroy(&queue->connection_lock);
    pthread_mutex_destroy(&queue->bootstrap_lock);
    pthread_cond_destroy(&queue->bootstrap_cond);
    free(queue);
    
    database_subsystem_shutdown();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_null_db_queue);
    RUN_TEST(test_non_lead_queue);
    RUN_TEST(test_lead_queue_no_connection);
    RUN_TEST(test_request_allocation_failure);
    RUN_TEST(test_query_id_allocation_failure);
    RUN_TEST(test_sql_template_allocation_failure);
    RUN_TEST(test_parameters_json_allocation_failure);
    RUN_TEST(test_query_execution_failure);
    RUN_TEST(test_successful_execution_no_qtc);
    RUN_TEST(test_successful_execution_with_qtc);
    RUN_TEST(test_migration_tracking_available);
    RUN_TEST(test_migration_tracking_installed);
    RUN_TEST(test_migration_tracking_mixed);
    RUN_TEST(test_bootstrap_completion_signaling);
    RUN_TEST(test_empty_database_detection);

    return UNITY_END();
}
