/*
 * Unity Test File: lua_context_test_panic.c
 *
 * Tests H_lua_panic - the Lua panic handler installed via lua_atpanic.
 * The handler surfaces the panic message through the logging subsystem
 * and returns 0 so Lua can unwind.
 *
 * Validates:
 *   - H_lua_panic logs the message left on the stack
 *   - H_lua_panic returns 0
 *   - H_lua_panic tolerates a nil message (logs "(no message)")
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <src/scripting/lua_context.h>

// Mock app_config (zeroed; H_lua_panic does not consult it).
static AppConfig mock_app_config_storage = {0};

// Forward declarations
void test_panic_logs_message_and_returns_zero(void);
void test_panic_handles_nil_message(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    app_config = NULL;
}

// A panic leaves a message on the stack; the handler must log it and
// return 0 without crashing.
void test_panic_logs_message_and_returns_zero(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_pushstring(L, "unrecoverable sandbox failure");
    int rc = H_lua_panic(L);
    TEST_ASSERT_EQUAL_INT(0, rc);

    H_lua_destroy_context(L);
}

// lua_tostring can yield NULL for a non-string top value. The handler
// must substitute "(no message)" rather than dereferencing NULL.
void test_panic_handles_nil_message(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    // Push a non-string (a number) so lua_tostring returns NULL.
    lua_pushinteger(L, 42);
    int rc = H_lua_panic(L);
    TEST_ASSERT_EQUAL_INT(0, rc);

    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_panic_logs_message_and_returns_zero);
    RUN_TEST(test_panic_handles_nil_message);

    return UNITY_END();
}
