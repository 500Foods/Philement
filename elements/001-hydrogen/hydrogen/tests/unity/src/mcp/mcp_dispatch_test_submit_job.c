/*
 * Unity Test File: mcp_dispatch_test_submit_job.c
 * Unit tests for mcp_dispatch_submit_job() in src/mcp/mcp_dispatch.c
 *
 * mcp_dispatch_submit_job is a 3-way dispatcher that selects a job
 * submission backend:
 *   1. submit_hook installed  -> delegate to the hook
 *   2. protocol_source set    -> scripting_submit_job_with_source
 *   3. otherwise              -> scripting_submit_job
 *
 * These tests exercise all three dispatch paths plus edge cases.
 * The hook path is tested with a fake hook (no scripting subsystem
 * required). The source and default paths initialize the real
 * scripting worker pool (same pattern as worker_pool_test_submit.c).
 */

#include <unity/mocks/mock_libmicrohttpd.h>
#include <src/hydrogen.h>
#include <unity.h>

#include <src/mcp/mcp_dispatch.h>
#include <src/scripting/scripting.h>
#include <src/scripting/scoreboard.h>
#include <src/scripting/worker_pool.h>
#include <src/scripting/script_registry.h>

#include <string.h>

/* Forward declarations (required for -Wmissing-prototypes) */
void test_mcp_dispatch_submit_job_delegates_to_hook(void);
void test_mcp_dispatch_submit_job_hook_null_params(void);
void test_mcp_dispatch_submit_job_hook_returns_null(void);
void test_mcp_dispatch_submit_job_hook_priority_over_source(void);
void test_mcp_dispatch_submit_job_protocol_source_path(void);
void test_mcp_dispatch_submit_job_default_path(void);
void test_mcp_dispatch_submit_job_no_subsystem_returns_null(void);

/* --- Fake submit hook state --- */
static const char *last_submit_name;
static const char *last_submit_params;
static const char *hook_return_str;

/*
 * Fake submit hook matching the McpDispatchSubmitFn signature.
 * Captures the arguments it received and returns a heap-allocated
 * copy of hook_return_str (or NULL when hook_return_str is NULL).
 */
static char *fake_submit_hook(const char *script_name, const char *params_json) {
    last_submit_name   = script_name;
    last_submit_params = params_json;
    if (hook_return_str) {
        return strdup(hook_return_str);
    }
    return NULL;
}

/* --- Scripting subsystem lifecycle helpers --- */
static void init_scripting_subsystem(void) {
    scripting_init_state();
    TEST_ASSERT_TRUE(scripting_workers_init(1));
}

static void cleanup_scripting_subsystem(void) {
    scripting_workers_destroy();
    scripting_cleanup_state();
    scripting_system_shutdown = 0;
    scripting_orchestrator_state = NULL;
}

void setUp(void) {
    mock_mhd_reset_all();
    mcp_dispatch_clear_hooks();
    last_submit_name   = NULL;
    last_submit_params = NULL;
    hook_return_str    = NULL;
    app_config         = NULL;
}

void tearDown(void) {
    mcp_dispatch_clear_hooks();
    mock_mhd_reset_all();
}

/*
 * Path 1: a submit hook is installed -> mcp_dispatch_submit_job must
 * delegate to it, passing both arguments through unchanged, and return
 * exactly what the hook returned.
 */
void test_mcp_dispatch_submit_job_delegates_to_hook(void) {
    char *result;

    mcp_dispatch_set_submit_hook(fake_submit_hook);
    hook_return_str = "job-123";

    result = mcp_dispatch_submit_job("Mcp.Server", "{\"message\":\"hello\"}");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("job-123", result);
    TEST_ASSERT_EQUAL_STRING("Mcp.Server", last_submit_name);
    TEST_ASSERT_EQUAL_STRING("{\"message\":\"hello\"}", last_submit_params);

    free(result);
}

/*
 * Path 1: NULL params_json is forwarded to the hook unchanged.
 */
void test_mcp_dispatch_submit_job_hook_null_params(void) {
    char *result;

    mcp_dispatch_set_submit_hook(fake_submit_hook);
    hook_return_str = "job-456";

    result = mcp_dispatch_submit_job("Mcp.Server", NULL);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("job-456", result);
    TEST_ASSERT_EQUAL_STRING("Mcp.Server", last_submit_name);
    TEST_ASSERT_NULL(last_submit_params);

    free(result);
}

/*
 * Path 1: a hook that returns NULL -> the NULL is propagated.
 */
void test_mcp_dispatch_submit_job_hook_returns_null(void) {
    char *result;

    mcp_dispatch_set_submit_hook(fake_submit_hook);
    hook_return_str = NULL;

    result = mcp_dispatch_submit_job("Mcp.Server", "{}");
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("Mcp.Server", last_submit_name);
}

/*
 * Hook takes priority over protocol_source: when both are set the
 * hook is invoked and the protocol_source path is skipped.
 * (Verified without a scripting subsystem: if the source path were
 * taken, scripting_submit_job_with_source would return NULL because
 * scripting_workers is NULL.)
 */
void test_mcp_dispatch_submit_job_hook_priority_over_source(void) {
    char *result;

    mcp_dispatch_set_submit_hook(fake_submit_hook);
    hook_return_str = "hook-result";
    mcp_dispatch_set_protocol_source("return 0\n");

    result = mcp_dispatch_submit_job("Mcp.Server", "{\"a\":1}");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hook-result", result);
    TEST_ASSERT_EQUAL_STRING("Mcp.Server", last_submit_name);

    free(result);
}

/*
 * Path 2: no submit hook, protocol_source set ->
 * mcp_dispatch_submit_job must call scripting_submit_job_with_source,
 * which registers the source inline and enqueues a job.
 */
void test_mcp_dispatch_submit_job_protocol_source_path(void) {
    char *result;

    init_scripting_subsystem();

    mcp_dispatch_set_protocol_source("return 0\n");

    result = mcp_dispatch_submit_job("Mcp.Server", "{\"a\":1}");
    TEST_ASSERT_NOT_NULL(result);

    free(result);
    cleanup_scripting_subsystem();
}

/*
 * Path 3: no submit hook, no protocol_source ->
 * mcp_dispatch_submit_job must call scripting_submit_job, which looks
 * up the script name in the registry and enqueues a job.
 */
void test_mcp_dispatch_submit_job_default_path(void) {
    char *result;

    init_scripting_subsystem();

    /* scripting_submit_job does not register source, so pre-register it */
    TEST_ASSERT_TRUE(script_registry_register(scripting_workers->registry,
                                              "Mcp.Server", "return 0"));

    result = mcp_dispatch_submit_job("Mcp.Server", "{\"a\":1}");
    TEST_ASSERT_NOT_NULL(result);

    free(result);
    cleanup_scripting_subsystem();
}

/*
 * Path 3 edge case: no hook, no source, scripting subsystem NOT
 * running -> scripting_submit_job returns NULL (scripting_workers is
 * NULL), so mcp_dispatch_submit_job returns NULL.
 */
void test_mcp_dispatch_submit_job_no_subsystem_returns_null(void) {
    char *result;

    result = mcp_dispatch_submit_job("Mcp.Server", "{}");
    TEST_ASSERT_NULL(result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mcp_dispatch_submit_job_delegates_to_hook);
    RUN_TEST(test_mcp_dispatch_submit_job_hook_null_params);
    RUN_TEST(test_mcp_dispatch_submit_job_hook_returns_null);
    RUN_TEST(test_mcp_dispatch_submit_job_hook_priority_over_source);
    RUN_TEST(test_mcp_dispatch_submit_job_protocol_source_path);
    RUN_TEST(test_mcp_dispatch_submit_job_default_path);
    RUN_TEST(test_mcp_dispatch_submit_job_no_subsystem_returns_null);
    return UNITY_END();
}
