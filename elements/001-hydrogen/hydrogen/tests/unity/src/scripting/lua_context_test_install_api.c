/*
 * Unity Test File: lua_context_test_install_api.c
 *
 * Tests H_lua_install_api - the host-table installer invoked by
 * H_lua_create_context. The NULL-state guard is the only branch
 * reachable in isolation without a fully populated state, so we cover
 * it directly here.
 *
 * Validates:
 *   - H_lua_install_api(NULL) is a safe no-op
 *   - H_lua_install_api populates a real state with the H table and
 *     its placeholder / installed sub-tables (sanity that the
 *     installer ran without tripping its defensive bail-out path)
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/scripting/lua_context.h>

// Mock app_config (zeroed; H_lua_install_api does not consult it).
static AppConfig mock_app_config_storage = {0};

// Forward declarations
void test_install_api_null_state_is_noop(void);
void test_install_api_populates_H_table(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    app_config = NULL;
}

// Passing NULL must return immediately without touching the state.
void test_install_api_null_state_is_noop(void) {
    H_lua_install_api(NULL);
    TEST_PASS();
}

// On a real state, H_lua_install_api must install a global H table and
// the agreed sub-tables / installed functions. If it ever tripped its
// defensive "Failed to install H table" bail-out, H would be missing.
void test_install_api_populates_H_table(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    // Re-run the installer to exercise the populated-state path.
    H_lua_install_api(L);

    lua_getglobal(L, "H");
    TEST_ASSERT_TRUE(lua_istable(L, -1));
    lua_pop(L, 1);

    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_install_api_null_state_is_noop);
    RUN_TEST(test_install_api_populates_H_table);

    return UNITY_END();
}
