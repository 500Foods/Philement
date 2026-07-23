/*
 * Unity Test File: scripting_api_query_resolve_test_resolve_db_queue
 *
 * Tests uncovered lines in scripting_api_query_resolve.c for resolve_db_queue.
 * Covers: no app_config, null global_queue_manager, database not found,
 * no database specified.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <unity/mocks/mock_dbqueue.h>

#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_api_internal.h>
#include <src/scripting/scripting_handle.h>
#include <src/scripting/lua_context.h>

extern DatabaseQueueManager* global_queue_manager;

void test_resolve_db_queue_no_app_config_returns_error(void);
void test_resolve_db_queue_null_global_queue_manager_returns_error(void);
void test_resolve_db_queue_database_not_found_returns_error(void);
void test_resolve_db_queue_no_database_fallback_returns_error(void);

void setUp(void) {
    mock_dbqueue_reset_all();

    app_config = NULL;
    global_queue_manager = NULL;
}

void tearDown(void) {
    app_config = NULL;
    global_queue_manager = NULL;
}

void test_resolve_db_queue_no_app_config_returns_error(void) {
    app_config = NULL;
    global_queue_manager = NULL;

    char* err_out = NULL;
    DatabaseQueue* result = resolve_db_queue(NULL, &err_out);

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    TEST_ASSERT_NOT_NULL(strstr(err_out, "no app_config"));
    free(err_out);
}

void test_resolve_db_queue_null_global_queue_manager_returns_error(void) {
    AppConfig config = {0};
    app_config = &config;
    global_queue_manager = NULL;

    config.scripting.DefaultDatabase = strdup("test-db");

    char* err_out = NULL;
    DatabaseQueue* result = resolve_db_queue("test-db", &err_out);

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    TEST_ASSERT_NOT_NULL(strstr(err_out, "queue manager not available"));
    free(config.scripting.DefaultDatabase);
    free(err_out);
}

void test_resolve_db_queue_database_not_found_returns_error(void) {
    AppConfig config = {0};
    memset(&config, 0, sizeof(config));
    config.scripting.DefaultDatabase = NULL;

    app_config = &config;

    DatabaseQueueManager mgr = {0};
    global_queue_manager = &mgr;

    mock_dbqueue_set_get_database_result(NULL);

    char* err_out = NULL;
    DatabaseQueue* result = resolve_db_queue("nonexistent-db", &err_out);

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    TEST_ASSERT_NOT_NULL(strstr(err_out, "not found"));
    free(err_out);
}

void test_resolve_db_queue_no_database_fallback_returns_error(void) {
    AppConfig config = {0};
    memset(&config, 0, sizeof(config));
    config.scripting.DefaultDatabase = NULL;
    config.databases.connection_count = 0;

    app_config = &config;

    char* err_out = NULL;
    DatabaseQueue* result = resolve_db_queue(NULL, &err_out);

    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(err_out);
    TEST_ASSERT_NOT_NULL(strstr(err_out, "no database specified"));
    free(err_out);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_resolve_db_queue_no_app_config_returns_error);
    RUN_TEST(test_resolve_db_queue_null_global_queue_manager_returns_error);
    RUN_TEST(test_resolve_db_queue_database_not_found_returns_error);
    RUN_TEST(test_resolve_db_queue_no_database_fallback_returns_error);

    return UNITY_END();
}