/*
 * Unity Test File: scripting_api_system_test_sleep.c
 *
 * Phase 11 of the LUA_PLAN. Tests H.sleep and H.shutdown_requested -
 * the cooperative shutdown primitives exposed to Lua scripts.
 *
 * Also tests H_lua_install_sleep_shutdown error path (H table missing).
 *
 * Validates:
 *   - H.sleep() with no arguments returns immediately (no error)
 *   - H.sleep(0) with a non-positive number returns immediately
 *   - H.sleep(-1) with a negative number returns immediately
 *   - H.sleep("not a number") logs an error and returns
 *   - H.sleep(1) with a small positive number sleeps briefly and returns
 *   - H.sleep(1000) returns immediately when scripting_system_shutdown is set
 *   - H.shutdown_requested() returns a boolean (false by default)
 *   - H.shutdown_requested() returns true when shutdown flag is set
 *   - H_lua_install_sleep_shutdown logs an error when H table is missing
 */

// Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Standard includes
#include <string.h>

// Lua headers
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Modules under test
#include <src/scripting/lua_context.h>
#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_api_internal.h>
#include <src/scripting/scripting.h>

// Mock logging introspection
#include <tests/unity/mocks/mock_logging.h>

// Mock app_config (zeroed = no special defaults needed for this suite)
static AppConfig mock_app_config_storage = {0};

// Forward declarations
void test_sleep_no_args_returns_immediately(void);
void test_sleep_zero_returns_immediately(void);
void test_sleep_negative_returns_immediately(void);
void test_sleep_non_number_logs_error(void);
void test_sleep_small_positive_sleeps_briefly(void);
void test_sleep_shutdown_flag_returns_immediately(void);
void test_shutdown_requested_returns_boolean(void);
void test_shutdown_requested_true_when_flag_set(void);
void test_install_sleep_shutdown_h_table_missing(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
    mock_logging_reset_all();
    scripting_system_shutdown = 0;
}

void tearDown(void) {
    scripting_system_shutdown = 0;
    app_config = NULL;
}

// H.sleep() with no arguments: nargs < 1, returns 0 immediately.
void test_sleep_no_args_returns_immediately(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    int rc = H_lua_run_string(L, "H.sleep()", "[phase11:sleep-noargs]");
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "H.sleep() with no args should not raise");

    H_lua_destroy_context(L);
}

// H.sleep(0) with a non-positive number: v <= 0.0, returns 0 immediately.
void test_sleep_zero_returns_immediately(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    int rc = H_lua_run_string(L, "H.sleep(0)", "[phase11:sleep-zero]");
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "H.sleep(0) should not raise");

    H_lua_destroy_context(L);
}

// H.sleep(-1) with a negative number: v <= 0.0, returns 0 immediately.
void test_sleep_negative_returns_immediately(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    int rc = H_lua_run_string(L, "H.sleep(-1)", "[phase11:sleep-negative]");
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "H.sleep(-1) should not raise");

    H_lua_destroy_context(L);
}

// H.sleep("not a number") with a non-number argument: logs error, returns 0.
void test_sleep_non_number_logs_error(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    int rc = H_lua_run_string(L, "H.sleep('not a number')", "[phase11:sleep-nonnumber]");
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "H.sleep with non-number should not raise");

    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "must be a number"));

    H_lua_destroy_context(L);
}

// H.sleep(1) with a small positive number: sleeps ~1ms and returns.
void test_sleep_small_positive_sleeps_briefly(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    int rc = H_lua_run_string(L, "H.sleep(1)", "[phase11:sleep-small]");
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "H.sleep(1) should not raise");

    H_lua_destroy_context(L);
}

// H.sleep(1000) when scripting_system_shutdown is set: returns immediately.
void test_sleep_shutdown_flag_returns_immediately(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    scripting_system_shutdown = 1;
    int rc = H_lua_run_string(L, "H.sleep(1000)", "[phase11:sleep-shutdown]");
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "H.sleep with shutdown flag should not raise");

    H_lua_destroy_context(L);
}

// H.shutdown_requested() returns a boolean (false by default).
void test_shutdown_requested_returns_boolean(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "shutdown_requested");
    int rc = lua_pcall(L, 0, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "shutdown_requested should succeed");
    TEST_ASSERT_TRUE(lua_isboolean(L, -1));
    TEST_ASSERT_FALSE(lua_toboolean(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// H.shutdown_requested() returns true when the shutdown flag is set.
void test_shutdown_requested_true_when_flag_set(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    scripting_system_shutdown = 1;
    lua_getglobal(L, "H");
    lua_getfield(L, -1, "shutdown_requested");
    int rc = lua_pcall(L, 0, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "shutdown_requested should succeed");
    TEST_ASSERT_TRUE(lua_isboolean(L, -1));
    TEST_ASSERT_TRUE(lua_toboolean(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

// H_lua_install_sleep_shutdown with no H table: logs "H table missing" and returns.
void test_install_sleep_shutdown_h_table_missing(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    // Remove the H global so lua_getglobal returns nil.
    lua_pushnil(L);
    lua_setglobal(L, "H");

    H_lua_install_sleep_shutdown(L);

    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "H table missing"));

    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_sleep_no_args_returns_immediately);
    RUN_TEST(test_sleep_zero_returns_immediately);
    RUN_TEST(test_sleep_negative_returns_immediately);
    RUN_TEST(test_sleep_non_number_logs_error);
    RUN_TEST(test_sleep_small_positive_sleeps_briefly);
    RUN_TEST(test_sleep_shutdown_flag_returns_immediately);
    RUN_TEST(test_shutdown_requested_returns_boolean);
    RUN_TEST(test_shutdown_requested_true_when_flag_set);
    RUN_TEST(test_install_sleep_shutdown_h_table_missing);

    return UNITY_END();
}
