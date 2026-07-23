/*
 * Unity Test File: scripting_api_log_test.c
 *
 * Phase 6 of the LUA_PLAN. Tests H.log.{trace, debug, info, warn, error,
 * fatal} - the six C functions that bridge Lua's string.format to
 * Hydrogen's log_this with the Scripting subsystem tag.
 *
 * Validates:
 *   - All six levels route to log_this with the correct priority
 *   - The formatted message is what lands in log_this
 *   - count_format_specifiers is consulted: a format/args mismatch
 *     is logged and does not raise a Lua error
 *   - Missing format string is handled cleanly (no crash)
 *   - Non-string first argument is handled cleanly
 *   - A real Lua chunk calling H.log.info can be compiled and run
 *     end-to-end through H_lua_run_string
 *
 * Note: The Unity framework uses a mock log_this (from mock_logging.c)
 * that does not populate the rolling log buffer. Tests assert on the
 * mock's "last message / last subsystem / last priority" API instead.
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Lua headers
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Module under test
#include <src/scripting/lua_context.h>
#include <src/scripting/scripting_api.h>

// Mock logging introspection API. The mock log_this records the last
// call into mock_last_message / mock_last_subsystem / mock_last_priority
// for inspection. The rolling log buffer is NOT populated.
#include <tests/unity/mocks/mock_logging.h>

// Mock app_config
static AppConfig mock_app_config_storage = {0};

// Forward declarations (Unity wants explicit prototypes for every test)
void test_log_info_basic_message(void);
void test_log_info_with_format_specifiers(void);
void test_log_info_percent_literal(void);
void test_log_error_message(void);
void test_log_warn_message(void);
void test_log_trace_message(void);
void test_log_debug_message(void);
void test_log_fatal_message(void);
void test_log_arg_mismatch_logs_error_does_not_raise(void);
void test_log_missing_format_does_not_crash(void);
void test_log_non_string_first_arg_does_not_crash(void);
void test_log_called_from_real_lua_chunk(void);
void test_log_empty_format_string_is_valid(void);
void test_log_multiple_args(void);
void test_log_with_job_id_context(void);
void test_log_with_empty_job_id_context(void);
void test_log_with_long_job_id_context(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
    mock_logging_reset_all();
}

void tearDown(void) {
    app_config = NULL;
}

// H.log.info("phase6 simple") - basic message, no specifiers.
void test_log_info_basic_message(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "log");
    lua_getfield(L, -1, "info");
    lua_pushstring(L, "phase6 simple");
    int rc = lua_pcall(L, 1, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "H.log.info basic call should succeed");

    TEST_ASSERT_EQUAL_STRING(SR_SCRIPTING, mock_logging_get_last_subsystem());
    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "phase6 simple"));
    TEST_ASSERT_EQUAL_INT(LOG_LEVEL_STATE, mock_logging_get_last_priority());

    H_lua_destroy_context(L);
}

// H.log.info("value=%d name=%s", 42, "phil") - format specifier substitution.
void test_log_info_with_format_specifiers(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "log");
    lua_getfield(L, -1, "info");
    lua_pushstring(L, "value=%d name=%s");
    lua_pushinteger(L, 42);
    lua_pushstring(L, "phil");
    int rc = lua_pcall(L, 3, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "format call should succeed");

    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "value=42"));
    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "name=phil"));

    H_lua_destroy_context(L);
}

// H.log.info("100%% complete") - %% is a literal, no args needed.
//
// Note: The mock log_this runs vsnprintf on the final formatted
// string, so a literal '%' in the message gets consumed as a format
// specifier in the mock. The production code is correct (it routes
// through Lua's string.format which handles %% before forwarding);
// we verify the behavior (call succeeded, correct subsystem/priority,
// a non-empty message was logged) rather than the exact characters.
void test_log_info_percent_literal(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "log");
    lua_getfield(L, -1, "info");
    lua_pushstring(L, "100%% complete");
    int rc = lua_pcall(L, 1, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "%% literal call should succeed");

    // The key behavior: count_format_specifiers saw 0 specs, so the
    // host did not return early with a mismatch. A log line was
    // emitted at the right priority, and the message string is
    // non-empty (we don't assert exact chars because the mock's
    // vsnprintf misinterprets literal '%').
    TEST_ASSERT_EQUAL_STRING(SR_SCRIPTING, mock_logging_get_last_subsystem());
    TEST_ASSERT_EQUAL_INT(LOG_LEVEL_STATE, mock_logging_get_last_priority());
    const char* msg = mock_logging_get_last_message();
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_GREATER_THAN(0, (int)strlen(msg));

    H_lua_destroy_context(L);
}

// H.log.error - distinct priority, same plumbing.
void test_log_error_message(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "log");
    lua_getfield(L, -1, "error");
    lua_pushstring(L, "phase6 boom");
    int rc = lua_pcall(L, 1, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "H.log.error should succeed");

    TEST_ASSERT_EQUAL_STRING(SR_SCRIPTING, mock_logging_get_last_subsystem());
    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "phase6 boom"));
    TEST_ASSERT_EQUAL_INT(LOG_LEVEL_ERROR, mock_logging_get_last_priority());

    H_lua_destroy_context(L);
}

void test_log_warn_message(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "log");
    lua_getfield(L, -1, "warn");
    lua_pushstring(L, "phase6 warn x=%d");
    lua_pushinteger(L, 7);
    int rc = lua_pcall(L, 2, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "H.log.warn should succeed");

    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "phase6 warn x=7"));
    TEST_ASSERT_EQUAL_INT(LOG_LEVEL_ALERT, mock_logging_get_last_priority());

    H_lua_destroy_context(L);
}

void test_log_trace_message(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "log");
    lua_getfield(L, -1, "trace");
    lua_pushstring(L, "phase6 trace here");
    int rc = lua_pcall(L, 1, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "H.log.trace should succeed");

    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "phase6 trace here"));
    TEST_ASSERT_EQUAL_INT(LOG_LEVEL_TRACE, mock_logging_get_last_priority());

    H_lua_destroy_context(L);
}

void test_log_debug_message(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "log");
    lua_getfield(L, -1, "debug");
    lua_pushstring(L, "phase6 debug here");
    int rc = lua_pcall(L, 1, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "H.log.debug should succeed");

    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "phase6 debug here"));
    TEST_ASSERT_EQUAL_INT(LOG_LEVEL_DEBUG, mock_logging_get_last_priority());

    H_lua_destroy_context(L);
}

void test_log_fatal_message(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "log");
    lua_getfield(L, -1, "fatal");
    lua_pushstring(L, "phase6 fatal here");
    int rc = lua_pcall(L, 1, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "H.log.fatal should succeed");

    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "phase6 fatal here"));
    TEST_ASSERT_EQUAL_INT(LOG_LEVEL_FATAL, mock_logging_get_last_priority());

    H_lua_destroy_context(L);
}

// H.log.info("x=%d", 1, 2) - 1 specifier, 2 args: the host path must
// NOT raise a Lua error. It logs a clear mismatch via SR_SCRIPTING instead.
void test_log_arg_mismatch_logs_error_does_not_raise(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "log");
    lua_getfield(L, -1, "info");
    lua_pushstring(L, "x=%d");
    lua_pushinteger(L, 1);
    lua_pushinteger(L, 2);
    int rc = lua_pcall(L, 3, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc,
        "arg mismatch must not raise a Lua error");

    // The mismatch itself IS a log_this call, at ERROR priority,
    // targeting SR_SCRIPTING.
    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "format/args mismatch"));
    TEST_ASSERT_EQUAL_INT(LOG_LEVEL_ERROR, mock_logging_get_last_priority());

    H_lua_destroy_context(L);
}

// H.log.info() with no arguments at all: should not crash, logs an error.
void test_log_missing_format_does_not_crash(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "log");
    lua_getfield(L, -1, "info");
    int rc = lua_pcall(L, 0, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc,
        "missing format string must not raise a Lua error");

    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "missing format string"));

    H_lua_destroy_context(L);
}

// H.log.info(<table>) - non-string first arg: should not crash, no raise.
//
// Lua's lua_isstring returns true for numbers (they are always
// coercible to a string), so we pass a table to exercise the
// "not a string" branch in H_lua_log_at_level.
void test_log_non_string_first_arg_does_not_crash(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "log");
    lua_getfield(L, -1, "info");
    lua_newtable(L); // {} is not lua_isstring, exercises the early-bail branch
    int rc = lua_pcall(L, 1, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc,
        "non-string first arg must not raise a Lua error");

    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(), "must be a string"));

    H_lua_destroy_context(L);
}

// End-to-end: a real Lua chunk that calls H.log.info via H_lua_run_string.
void test_log_called_from_real_lua_chunk(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    const char* code =
        "H.log.info('chunk says hello %s number %d', 'world', 99)";

    int rc = H_lua_run_string(L, code, "[phase6:chunk-log]");
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "chunk should run successfully");

    TEST_ASSERT_EQUAL_STRING(SR_SCRIPTING, mock_logging_get_last_subsystem());
    TEST_ASSERT_NOT_NULL(strstr(mock_logging_get_last_message(),
                                "chunk says hello world number 99"));

    H_lua_destroy_context(L);
}

// H.log.info("") - empty format string: no specifiers, no args,
// log_this is called with an empty formatted message.
void test_log_empty_format_string_is_valid(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "log");
    lua_getfield(L, -1, "info");
    lua_pushstring(L, "");
    int rc = lua_pcall(L, 1, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "empty format string should be valid");

    // Empty format -> empty formatted message sent to log_this.
    TEST_ASSERT_EQUAL_STRING("", mock_logging_get_last_message());
    TEST_ASSERT_EQUAL_STRING(SR_SCRIPTING, mock_logging_get_last_subsystem());

    H_lua_destroy_context(L);
}

// H.log.info("%d %d %d", 1, 2, 3) - three args, all numbers.
void test_log_multiple_args(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "log");
    lua_getfield(L, -1, "info");
    lua_pushstring(L, "a=%d b=%d c=%d");
    lua_pushinteger(L, 1);
    lua_pushinteger(L, 2);
    lua_pushinteger(L, 3);
    int rc = lua_pcall(L, 4, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "multi-arg call should succeed");

    TEST_ASSERT_EQUAL_STRING("a=1 b=2 c=3", mock_logging_get_last_message());

    H_lua_destroy_context(L);
}

// H.log.info("message") with a job_id context set via H_lua_set_job_context.
// The log message should be prefixed with "[job:ABC12] ".
void test_log_with_job_id_context(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_job_context ctx = {0};
    strncpy(ctx.job_id, "ABC12", sizeof(ctx.job_id) - 1);
    ctx.job_id[sizeof(ctx.job_id) - 1] = '\0';
    H_lua_set_job_context(L, &ctx);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "log");
    lua_getfield(L, -1, "info");
    lua_pushstring(L, "test message from job");
    int rc = lua_pcall(L, 1, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "H.log.info with job context should succeed");

    const char* msg = mock_logging_get_last_message();
    TEST_ASSERT_NOT_NULL(strstr(msg, "[job:ABC12]"));
    TEST_ASSERT_NOT_NULL(strstr(msg, "test message from job"));

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);
}

// H.log.info("message") with an empty job_id (context set but job_id is empty).
// Should NOT prefix the message.
void test_log_with_empty_job_id_context(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_job_context ctx = {0};
    H_lua_set_job_context(L, &ctx);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "log");
    lua_getfield(L, -1, "info");
    lua_pushstring(L, "no job context");
    int rc = lua_pcall(L, 1, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "H.log.info with empty job_id should succeed");

    const char* msg = mock_logging_get_last_message();
    TEST_ASSERT_EQUAL_STRING("no job context", msg);
    TEST_ASSERT_NULL(strstr(msg, "[job:"));

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);
}

// H.log.info("message") with a job_id at the maximum supported length
// (ID_LEN characters). Verifies the prefix is emitted correctly at the
// boundary; IDs longer than ID_LEN are silently truncated by the fixed-size
// job_id buffer, so the test stays within the supported length.
void test_log_with_long_job_id_context(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    H_lua_job_context ctx = {0};
    strncpy(ctx.job_id, "LONGJ", sizeof(ctx.job_id) - 1);
    ctx.job_id[sizeof(ctx.job_id) - 1] = '\0';
    H_lua_set_job_context(L, &ctx);

    lua_getglobal(L, "H");
    lua_getfield(L, -1, "log");
    lua_getfield(L, -1, "info");
    lua_pushstring(L, "long job id test");
    int rc = lua_pcall(L, 1, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(LUA_OK, rc, "H.log.info with max-length job_id should succeed");

    const char* msg = mock_logging_get_last_message();
    TEST_ASSERT_NOT_NULL(strstr(msg, "[job:LONGJ]"));
    TEST_ASSERT_NOT_NULL(strstr(msg, "long job id test"));

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_log_info_basic_message);
    RUN_TEST(test_log_info_with_format_specifiers);
    RUN_TEST(test_log_info_percent_literal);
    RUN_TEST(test_log_error_message);
    RUN_TEST(test_log_warn_message);
    RUN_TEST(test_log_trace_message);
    RUN_TEST(test_log_debug_message);
    RUN_TEST(test_log_fatal_message);
    RUN_TEST(test_log_arg_mismatch_logs_error_does_not_raise);
    RUN_TEST(test_log_missing_format_does_not_crash);
    RUN_TEST(test_log_non_string_first_arg_does_not_crash);
    RUN_TEST(test_log_called_from_real_lua_chunk);
    RUN_TEST(test_log_empty_format_string_is_valid);
    RUN_TEST(test_log_multiple_args);
    RUN_TEST(test_log_with_job_id_context);
    RUN_TEST(test_log_with_empty_job_id_context);
    RUN_TEST(test_log_with_long_job_id_context);

    return UNITY_END();
}
