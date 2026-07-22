/*
 * Unity Test File: scripting_api_llm_test_resolve_db_queue.c
 *
 * Tests resolve_llm_db_queue() helper function for error paths:
 *   - NULL app_config when db_name is NULL/empty
 *   - NULL global_queue_manager
 *   - Database not found
 *   - Fallback to first database connection
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#define USE_MOCK_DBQUEUE
#include <unity/mocks/mock_dbqueue.h>

#include <src/scripting/scripting_api_llm.h>
#include <src/database/dbqueue/dbqueue.h>

/* Forward declarations */
void test_resolve_db_queue_null_app_config_returns_error(void);
void test_resolve_db_queue_null_global_queue_manager_returns_error(void);
void test_resolve_db_queue_database_not_found_returns_error(void);
void test_resolve_db_queue_fallback_to_first_connection(void);
void test_resolve_db_queue_with_valid_database(void);

/* Declare global_queue_manager as extern - it's defined in the source */
extern DatabaseQueueManager* global_queue_manager;

void setUp(void) {
    mock_dbqueue_reset_all();
}

void tearDown(void) {
}

/* resolve_llm_db_queue with NULL app_config returns error */
void test_resolve_db_queue_null_app_config_returns_error(void) {
    app_config = NULL;
    global_queue_manager = NULL;

    char* err_out = (char*)0xDEADBEEF; /* Should be overwritten */
    DatabaseQueue* result = resolve_llm_db_queue(NULL, &err_out);

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    TEST_ASSERT_TRUE(strstr(err_out, "no app_config") != NULL || strstr(err_out, "no database specified") != NULL);
    free(err_out);
}

/* resolve_llm_db_queue with NULL global_queue_manager returns error */
void test_resolve_db_queue_null_global_queue_manager_returns_error(void) {
    AppConfig config = {0};
    app_config = &config;
    global_queue_manager = NULL;

    char* err_out = NULL;
    DatabaseQueue* result = resolve_llm_db_queue("test-db", &err_out);

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    TEST_ASSERT_TRUE(strstr(err_out, "queue manager not available") != NULL);
    free(err_out);
}

/* resolve_llm_db_queue with database not found returns error */
void test_resolve_db_queue_database_not_found_returns_error(void) {
    AppConfig config = {0};
    memset(&config, 0, sizeof(config));
    config.scripting.DefaultDatabase = NULL;
    app_config = &config;

    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;

    mock_dbqueue_set_get_database_result(NULL);

    char* err_out = NULL;
    DatabaseQueue* result = resolve_llm_db_queue("nonexistent-db", &err_out);

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    TEST_ASSERT_TRUE(strstr(err_out, "not found") != NULL);
    free(err_out);
}

/* resolve_llm_db_queue falls back to first database connection when no db_name provided */
void test_resolve_db_queue_fallback_to_first_connection(void) {
    AppConfig config = {0};
    memset(&config, 0, sizeof(config));
    config.scripting.DefaultDatabase = NULL;
    config.databases.connection_count = 1;
    config.databases.connections[0].name = strdup("default-db");

    app_config = &config;

    DatabaseQueue dbq = {0};

    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;

    mock_dbqueue_set_get_database_result(&dbq);

    char* err_out = NULL;
    DatabaseQueue* result = resolve_llm_db_queue(NULL, &err_out);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result == &dbq);
}

/* resolve_llm_db_queue with valid database returns queue */
void test_resolve_db_queue_with_valid_database(void) {
    AppConfig config = {0};
    config.scripting.DefaultDatabase = strdup("my-db");
    config.databases.connection_count = 0;

    app_config = &config;

    DatabaseQueue dbq = {0};

    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;

    mock_dbqueue_set_get_database_result(&dbq);

    char* err_out = NULL;
    DatabaseQueue* result = resolve_llm_db_queue("my-db", &err_out);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result == &dbq);

    free(config.scripting.DefaultDatabase);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_resolve_db_queue_null_app_config_returns_error);
    RUN_TEST(test_resolve_db_queue_null_global_queue_manager_returns_error);
    RUN_TEST(test_resolve_db_queue_database_not_found_returns_error);
    RUN_TEST(test_resolve_db_queue_fallback_to_first_connection);
    RUN_TEST(test_resolve_db_queue_with_valid_database);

    return UNITY_END();
}