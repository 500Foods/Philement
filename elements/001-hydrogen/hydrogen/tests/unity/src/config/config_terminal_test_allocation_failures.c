/*
 * Unity Test File: Terminal Configuration Allocation Failure Tests
 * Tests load_terminal_config error paths when strdup calls fail.
 * Uses mock_system to simulate allocation failures at each strdup call
 * in the initialization block of load_terminal_config.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/config/config_terminal.h>
#include <src/config/config.h>

// Include mock_system to control strdup failure injection.
// USE_MOCK_SYSTEM is not defined by CMake for terminal test files,
// so we define it here with an #ifndef guard. This makes strdup in
// THIS test file map to mock_strdup, so json_object()/json_decref()
// calls here also use the mock — but we reset the counter after
// setting the failure point, so only strdup calls inside
// load_terminal_config (in the source object, which is also
// compiled with USE_MOCK_SYSTEM) are counted.
#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_logging.h>

// Forward declarations for functions being tested
bool load_terminal_config(json_t* root, AppConfig* config);
void cleanup_terminal_config(TerminalConfig* config);

// Forward declarations for test functions
void test_load_terminal_config_strdup_fail_web_path(void);
void test_load_terminal_config_strdup_fail_shell_command_zsh(void);
void test_load_terminal_config_strdup_fail_webroot(void);
void test_load_terminal_config_strdup_fail_index_page(void);
void test_load_terminal_config_strdup_fail_protocol(void);
void test_load_terminal_config_strdup_fail_key(void);
void test_load_terminal_config_all_allocations_succeed(void);

// Test setup and teardown
void setUp(void) {
    mock_system_reset_all();
    mock_logging_reset_all();
    unsetenv("WEBSOCKET_TERMINAL_KEY");
}

void tearDown(void) {
    mock_system_reset_all();
    mock_logging_reset_all();
    unsetenv("WEBSOCKET_TERMINAL_KEY");
}

// ===== ALLOCATION FAILURE TESTS =====
//
// In load_terminal_config, the strdup calls in the init block happen
// in this order (before any PROCESS_ macros):
//   call 1: strdup("/terminal")            -> web_path
//   call 2: strdup("/bin/zsh")            -> shell_command
//   call 3: strdup("/bin/bash")            -> fallback (only if call 2 fails)
//   call 4: strdup("PAYLOAD:/terminal")   -> webroot  (call 3 is skipped if zsh succeeds)
//   call 5: strdup("*")                    -> cors_origin
//   call 6: strdup("terminal.html")       -> index_page
//   call 7: strdup("terminal")             -> protocol
//   call 8: strdup("${env.WEBSOCKET_TERMINAL_KEY}") -> key
//
// When zsh (call 2) succeeds, the bash fallback (call 3) is never reached,
// so the numbering shifts: webroot becomes call 3, cors_origin call 4, etc.
//
// By setting mock_system_set_malloc_failure(N), the Nth call to
// mock_strdup (which shares the mock_malloc_call_count counter)
// returns NULL.  All other calls succeed normally.

// Fail on first strdup (web_path allocation) — covers lines 25-26
void test_load_terminal_config_strdup_fail_web_path(void) {
    AppConfig config = {0};
    json_t* root = json_object();
    TEST_ASSERT_NOT_NULL(root);

    // Reset counter AFTER json_object so its allocations don't count
    mock_system_set_malloc_failure(1);

    bool result = load_terminal_config(root, &config);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_TRUE(mock_logging_message_contains("Failed to allocate web path string"));

    json_decref(root);
    mock_system_reset_all();
}

// Fail on second strdup (shell_command/zsh) — covers line 32 (bash fallback)
// The bash fallback strdup (call 3) succeeds since failure is at call 2 only.
void test_load_terminal_config_strdup_fail_shell_command_zsh(void) {
    AppConfig config = {0};
    json_t* root = json_object();
    TEST_ASSERT_NOT_NULL(root);

    // Fail on call 2 (strdup("/bin/zsh")).  Call 3 (strdup("/bin/bash"))
    // succeeds, exercising the fallback path on line 32.
    mock_system_set_malloc_failure(2);

    bool result = load_terminal_config(root, &config);

    // The bash fallback succeeds, so the function continues past shell_command
    // and eventually reaches the webroot strdup (call 4).  Since we only
    // failed at call 2, all other allocations succeed and the function
    // returns true (with default JSON values since root is empty).
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("/bin/bash", config.terminal.shell_command);

    json_decref(root);
    cleanup_terminal_config(&config.terminal);
    mock_system_reset_all();
}

// Fail on third strdup (webroot) — covers lines 44-49
// When zsh succeeds (call 2), the bash fallback (call 3) is skipped,
// so webroot becomes call 3.
void test_load_terminal_config_strdup_fail_webroot(void) {
    AppConfig config = {0};
    json_t* root = json_object();
    TEST_ASSERT_NOT_NULL(root);

    // Calls: 1=web_path (succeeds), 2=shell_command (succeeds), 3=webroot (fails)
    mock_system_set_malloc_failure(3);

    bool result = load_terminal_config(root, &config);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_TRUE(mock_logging_message_contains("Failed to allocate webroot string"));
    TEST_ASSERT_NULL(config.terminal.web_path);
    TEST_ASSERT_NULL(config.terminal.shell_command);
    TEST_ASSERT_NULL(config.terminal.webroot);

    json_decref(root);
    mock_system_reset_all();
}

// Fail on fifth strdup (index_page) — covers lines 56-65
void test_load_terminal_config_strdup_fail_index_page(void) {
    AppConfig config = {0};
    json_t* root = json_object();
    TEST_ASSERT_NOT_NULL(root);

    // Calls: 1=web_path, 2=shell_command, 3=webroot, 4=cors_origin, 5=index_page
    mock_system_set_malloc_failure(5);

    bool result = load_terminal_config(root, &config);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_TRUE(mock_logging_message_contains("Failed to allocate index page string"));
    TEST_ASSERT_NULL(config.terminal.web_path);
    TEST_ASSERT_NULL(config.terminal.shell_command);
    TEST_ASSERT_NULL(config.terminal.webroot);
    TEST_ASSERT_NULL(config.terminal.cors_origin);
    TEST_ASSERT_NULL(config.terminal.index_page);

    json_decref(root);
    mock_system_reset_all();
}

// Fail on sixth strdup (protocol) — covers lines 70-81
void test_load_terminal_config_strdup_fail_protocol(void) {
    AppConfig config = {0};
    json_t* root = json_object();
    TEST_ASSERT_NOT_NULL(root);

    // Calls: 1=web_path, 2=shell_command, 3=webroot, 4=cors_origin,
    //        5=index_page, 6=protocol (fails)
    mock_system_set_malloc_failure(6);

    bool result = load_terminal_config(root, &config);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_TRUE(mock_logging_message_contains("Failed to allocate protocol string"));
    TEST_ASSERT_NULL(config.terminal.web_path);
    TEST_ASSERT_NULL(config.terminal.shell_command);
    TEST_ASSERT_NULL(config.terminal.webroot);
    TEST_ASSERT_NULL(config.terminal.cors_origin);
    TEST_ASSERT_NULL(config.terminal.index_page);
    TEST_ASSERT_NULL(config.terminal.protocol);

    json_decref(root);
    mock_system_reset_all();
}

// Fail on seventh strdup (key) — covers lines 86-99
void test_load_terminal_config_strdup_fail_key(void) {
    AppConfig config = {0};
    json_t* root = json_object();
    TEST_ASSERT_NOT_NULL(root);

    // Calls: 1=web_path, 2=shell_command, 3=webroot, 4=cors_origin,
    //        5=index_page, 6=protocol, 7=key (fails)
    mock_system_set_malloc_failure(7);

    bool result = load_terminal_config(root, &config);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_TRUE(mock_logging_message_contains("Failed to allocate key string"));
    TEST_ASSERT_NULL(config.terminal.web_path);
    TEST_ASSERT_NULL(config.terminal.shell_command);
    TEST_ASSERT_NULL(config.terminal.webroot);
    TEST_ASSERT_NULL(config.terminal.cors_origin);
    TEST_ASSERT_NULL(config.terminal.index_page);
    TEST_ASSERT_NULL(config.terminal.protocol);
    TEST_ASSERT_NULL(config.terminal.key);

    json_decref(root);
    mock_system_reset_all();
}

// Verify all allocations succeed with no mock failure — confirms normal path
void test_load_terminal_config_all_allocations_succeed(void) {
    AppConfig config = {0};
    json_t* root = json_object();
    TEST_ASSERT_NOT_NULL(root);

    // No mock failure set — all allocations succeed
    bool result = load_terminal_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("/terminal", config.terminal.web_path);
    TEST_ASSERT_EQUAL_STRING("/bin/zsh", config.terminal.shell_command);
    TEST_ASSERT_EQUAL_STRING("PAYLOAD:/terminal", config.terminal.webroot);
    TEST_ASSERT_EQUAL_STRING("*", config.terminal.cors_origin);
    TEST_ASSERT_EQUAL_STRING("terminal.html", config.terminal.index_page);
    TEST_ASSERT_EQUAL_STRING("terminal", config.terminal.protocol);
    TEST_ASSERT_EQUAL_STRING("${env.WEBSOCKET_TERMINAL_KEY}", config.terminal.key);

    json_decref(root);
    cleanup_terminal_config(&config.terminal);
    mock_system_reset_all();
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_load_terminal_config_strdup_fail_web_path);
    RUN_TEST(test_load_terminal_config_strdup_fail_shell_command_zsh);
    RUN_TEST(test_load_terminal_config_strdup_fail_webroot);
    RUN_TEST(test_load_terminal_config_strdup_fail_index_page);
    RUN_TEST(test_load_terminal_config_strdup_fail_protocol);
    RUN_TEST(test_load_terminal_config_strdup_fail_key);
    RUN_TEST(test_load_terminal_config_all_allocations_succeed);

    return UNITY_END();
}
