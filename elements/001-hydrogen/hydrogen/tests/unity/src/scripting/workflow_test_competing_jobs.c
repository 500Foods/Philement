/*
 * Unity Test File: workflow_test_competing_jobs.c
 *
 * Phase 27 of the LUA_PLAN. Tests the workflow stage pattern:
 *   - H.set_current_state updates the scoreboard entry's current_state field
 *   - H.set_result records artifact metadata (result_type, result_location)
 *   - H.notify.send returns the expected deferred error
 *   - Scoreboard JSON visibility for workflow status/metadata
 *
 * The atomic claim pattern (affected_rows == 1 vs 0) requires a real DB
 * and is validated by the blackbox test (test_43_scripting.sh extension).
 * This Unity test validates the scoreboard-side primitives that make
 * the claim pattern possible.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <time.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <jansson.h>

#include <src/scripting/scripting.h>
#include <src/scripting/scoreboard.h>
#include <src/scripting/scoreboard_json.h>
#include <src/scripting/worker_pool.h>
#include <src/scripting/lua_context.h>
#include <src/scripting/scripting_api.h>
#include <src/scripting/scripting_handle.h>
#include <tests/unity/mocks/mock_logging.h>

// Forward declarations (required for -Wmissing-prototypes)
void test_set_current_state_updates_scoreboard_entry(void);
void test_set_result_records_workflow_metadata(void);
void test_set_result_overwrites_previous_metadata(void);
void test_set_result_empty_clears_metadata(void);
void test_set_result_end_to_end_through_worker_pool(void);
void test_set_result_strings_survive_lua_gc(void);
void test_workflow_job_with_intentional_failure(void);
void test_notify_send_returns_error_handle(void);
void test_mail_send_returns_error_handle(void);
void test_notify_send_sync_returns_nil_error(void);
void test_mail_send_sync_returns_nil_error(void);
void test_scoreboard_json_emits_current_state(void);

#define POLL_TIMEOUT_MS 5000
#define POLL_SLEEP_USEC 1000

static ScoreboardJobStatus wait_for_terminal(const char* job_id,
                                             ScoreboardEntry** out_entry) {
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    while (1) {
        ScoreboardEntry* e = scoreboard_find(scripting_scoreboard, job_id);
        if (e) {
            if (e->status == SCOREBOARD_JOB_COMPLETED
                || e->status == SCOREBOARD_JOB_FAILED
                || e->status == SCOREBOARD_JOB_KILLED) {
                ScoreboardJobStatus s = e->status;
                if (out_entry) {
                    *out_entry = e;
                } else {
                    scoreboard_entry_free(e);
                }
                return s;
            }
            scoreboard_entry_free(e);
        }
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000
                        + (now.tv_nsec - start.tv_nsec) / 1000000;
        if (elapsed_ms >= POLL_TIMEOUT_MS) {
            TEST_FAIL_MESSAGE("Timed out waiting for job to reach terminal status");
        }
        usleep(POLL_SLEEP_USEC);
    }
}

static AppConfig mock_app_config_storage = {0};

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
    mock_logging_reset_all();
    scripting_init_state();
}

void tearDown(void) {
    scripting_workers_destroy();
    scripting_cleanup_state();
    scripting_system_shutdown = 0;
    scripting_orchestrator_state = NULL;
    app_config = NULL;
}

void test_set_current_state_updates_scoreboard_entry(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "workflow_step", NULL);
    TEST_ASSERT_NOT_NULL(id);

    H_lua_job_context ctx = {0};
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", id);
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    int rc = H_lua_run_string(L,
        "H.set_current_state('claiming workflow WF-001')\n"
        "H.set_current_state('processing step 1')\n"
        "H.set_current_state('completed step 1')\n",
        "[phase27:state]");
    TEST_ASSERT_EQUAL_INT(LUA_OK, rc);

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->current_state);
    TEST_ASSERT_EQUAL_STRING("completed step 1", e->current_state);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

void test_set_result_records_workflow_metadata(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "workflow_complete", NULL);
    TEST_ASSERT_NOT_NULL(id);

    H_lua_job_context ctx = {0};
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", id);
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    int rc = H_lua_run_string(L,
        "H.set_current_state('completing workflow')\n"
        "H.set_result('workflow', 'workflow:WF-001')\n",
        "[phase27:result]");
    TEST_ASSERT_EQUAL_INT(LUA_OK, rc);

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->current_state);
    TEST_ASSERT_EQUAL_STRING("completing workflow", e->current_state);
    TEST_ASSERT_NOT_NULL(e->result_type);
    TEST_ASSERT_EQUAL_STRING("workflow", e->result_type);
    TEST_ASSERT_NOT_NULL(e->result_location);
    TEST_ASSERT_EQUAL_STRING("workflow:WF-001", e->result_location);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

void test_set_result_overwrites_previous_metadata(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "overwrite_test", NULL);

    H_lua_job_context ctx = {0};
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", id);
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    int rc = H_lua_run_string(L,
        "H.set_result('json', 'inline:report:v1')\n"
        "H.set_result('workflow', 'workflow:WF-001')\n"
        "H.set_result('pdf', 's3://bucket/report.pdf')\n",
        "[phase27:overwrite]");
    TEST_ASSERT_EQUAL_INT(LUA_OK, rc);

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->result_type);
    TEST_ASSERT_EQUAL_STRING("pdf", e->result_type);
    TEST_ASSERT_EQUAL_STRING("s3://bucket/report.pdf", e->result_location);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

void test_set_result_empty_clears_metadata(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "clear_test", NULL);
    scoreboard_update_result(sb, id, "json", "inline:data");

    H_lua_job_context ctx = {0};
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", id);
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    int rc = H_lua_run_string(L,
        "H.set_result('', '')\n",
        "[phase27:clear]");
    TEST_ASSERT_EQUAL_INT(LUA_OK, rc);

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NULL(e->result_type);
    TEST_ASSERT_NULL(e->result_location);
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

void test_set_result_end_to_end_through_worker_pool(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    char* id = scripting_submit_job_with_source("phase27_worker",
        "H.set_current_state('initializing')\n"
        "H.set_result('workflow', 'workflow:WF-001')\n"
        "H.set_current_state('finalizing')\n"
        "return 0\n",
        NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = NULL;
    ScoreboardJobStatus s = wait_for_terminal(id, &e);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_COMPLETED, s);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->current_state);
    TEST_ASSERT_EQUAL_STRING("finalizing", e->current_state);
    TEST_ASSERT_NOT_NULL(e->result_type);
    TEST_ASSERT_EQUAL_STRING("workflow", e->result_type);
    TEST_ASSERT_NOT_NULL(e->result_location);
    TEST_ASSERT_EQUAL_STRING("workflow:WF-001", e->result_location);
    scoreboard_entry_free(e);
    free(id);
}

void test_set_result_strings_survive_lua_gc(void) {
    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "gc_survival", NULL);

    lua_State* L = H_lua_create_context();
    H_lua_job_context ctx = {0};
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", id);
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    int rc = H_lua_run_string(L,
        "do\n"
        "  local t = 'type-' .. string.rep('T', 256)\n"
        "  local l = 'location-' .. string.rep('L', 256)\n"
        "  H.set_result(t, l)\n"
        "end\n"
        "collectgarbage('collect')\n",
        "[phase27:gc_survival]");
    TEST_ASSERT_EQUAL_INT(LUA_OK, rc);

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);

    ScoreboardEntry* e = scoreboard_find(sb, id);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->result_type);
    TEST_ASSERT_NOT_NULL(strstr(e->result_type, "type-"));
    TEST_ASSERT_NOT_NULL(e->result_location);
    TEST_ASSERT_NOT_NULL(strstr(e->result_location, "location-"));
    scoreboard_entry_free(e);
    free(id);
    scoreboard_destroy(sb);
}

void test_workflow_job_with_intentional_failure(void) {
    TEST_ASSERT_TRUE(scripting_workers_init(2));
    char* id = scripting_submit_job_with_source("failing_workflow",
        "H.set_current_state('attempting step')\n"
        "error('simulated workflow failure')\n",
        NULL);
    TEST_ASSERT_NOT_NULL(id);

    ScoreboardEntry* e = NULL;
    ScoreboardJobStatus s = wait_for_terminal(id, &e);
    TEST_ASSERT_EQUAL(SCOREBOARD_JOB_FAILED, s);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_NOT_NULL(e->current_state);
    TEST_ASSERT_EQUAL_STRING("attempting step", e->current_state);
    TEST_ASSERT_NOT_NULL(e->error_message);
    TEST_ASSERT_NOT_NULL(strstr(e->error_message, "simulated workflow failure"));
    TEST_ASSERT_NOT_NULL(e->error_traceback);
    scoreboard_entry_free(e);
    free(id);
}

void test_notify_send_returns_error_handle(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "notify_test", NULL);

    H_lua_job_context ctx = {0};
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", id);
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    int rc = H_lua_run_string(L,
        "local handle = H.notify.send({ channel='sms', to='test', body='hello' })\n"
        "return handle\n",
        "[phase27:notify]");
    TEST_ASSERT_EQUAL_INT(LUA_OK, rc);
    TEST_ASSERT_NOT_NULL(lua_touserdata(L, -1));

    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_EQUAL_INT(H_HK_NOTIFY, h->kind);
    TEST_ASSERT_NOT_NULL(h->notify_error);
    TEST_ASSERT_EQUAL_STRING("notify: deferred to mailrelay rules", h->notify_error);

    lua_pop(L, 1);

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);
    free(id);
    scoreboard_destroy(sb);
}

void test_mail_send_returns_error_handle(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    Scoreboard* sb = scoreboard_create();
    char* id = scoreboard_submit(sb, "mail_test", NULL);

    H_lua_job_context ctx = {0};
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", id);
    ctx.scoreboard = sb;
    H_lua_set_job_context(L, &ctx);

    /* Raw subject/body without template is rejected (template-first policy). */
    int rc = H_lua_run_string(L,
        "local handle = H.mail.send({ to='test@example.com', subject='test', body='hello' })\n"
        "return handle\n",
        "[phase27:mail]");
    TEST_ASSERT_EQUAL_INT(LUA_OK, rc);
    TEST_ASSERT_NOT_NULL(lua_touserdata(L, -1));

    H_Handle* h = H_Handle_check(L, -1);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_EQUAL_INT(H_HK_MAIL, h->kind);
    TEST_ASSERT_NOT_NULL(h->mail_error);
    TEST_ASSERT_NOT_NULL(strstr(h->mail_error, "MAIL_PARAM_MISSING"));
    TEST_ASSERT_NOT_NULL(strstr(h->mail_error, "template"));

    lua_pop(L, 1);

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);
    free(id);
    scoreboard_destroy(sb);
}

void test_notify_send_sync_returns_nil_error(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    int rc = H_lua_run_string(L,
        "local result, err = H.notify.send_sync({ channel='sms', to='test', body='hello' })\n"
        "return result, err\n",
        "[phase27:notify_sync]");
    TEST_ASSERT_EQUAL_INT(LUA_OK, rc);

    TEST_ASSERT_EQUAL_INT(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL_INT(LUA_TSTRING, lua_type(L, -1));
    const char* err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_EQUAL_STRING("notify: deferred to mailrelay rules", err);

    lua_pop(L, 2);
    H_lua_destroy_context(L);
}

void test_mail_send_sync_returns_nil_error(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    /* Raw subject/body without template is rejected (template-first policy). */
    int rc = H_lua_run_string(L,
        "local result, err = H.mail.send_sync({ to='test@example.com', subject='test', body='hello' })\n"
        "return result, err\n",
        "[phase27:mail_sync]");
    TEST_ASSERT_EQUAL_INT(LUA_OK, rc);

    TEST_ASSERT_EQUAL_INT(LUA_TNIL, lua_type(L, -2));
    TEST_ASSERT_EQUAL_INT(LUA_TSTRING, lua_type(L, -1));
    const char* err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(err);
    TEST_ASSERT_NOT_NULL(strstr(err, "MAIL_PARAM_MISSING"));
    TEST_ASSERT_NOT_NULL(strstr(err, "template"));

    lua_pop(L, 2);
    H_lua_destroy_context(L);
}

void test_scoreboard_json_emits_current_state(void) {
    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    // Use the global scripting_scoreboard because the JSON snapshot
    // function reads that global state.
    char* id = scoreboard_submit(scripting_scoreboard, "json_test", NULL);
    TEST_ASSERT_NOT_NULL(id);

    H_lua_job_context ctx = {0};
    snprintf(ctx.job_id, sizeof(ctx.job_id), "%s", id);
    ctx.scoreboard = scripting_scoreboard;
    H_lua_set_job_context(L, &ctx);

    int rc = H_lua_run_string(L,
        "H.set_current_state('updated by json test')\n",
        "[phase27:json]");
    TEST_ASSERT_EQUAL_INT(LUA_OK, rc);

    H_lua_set_job_context(L, NULL);
    H_lua_destroy_context(L);

    json_t* snapshot = scripting_scoreboard_snapshot_json(10, false);
    TEST_ASSERT_NOT_NULL(snapshot);

    const char* json_str = json_dumps(snapshot, JSON_COMPACT);
    TEST_ASSERT_NOT_NULL(json_str);
    TEST_ASSERT_NOT_NULL(strstr(json_str, "updated by json test"));

    free((void*)json_str);
    json_decref(snapshot);

    free(id);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_set_current_state_updates_scoreboard_entry);
    RUN_TEST(test_set_result_records_workflow_metadata);
    RUN_TEST(test_set_result_overwrites_previous_metadata);
    RUN_TEST(test_set_result_empty_clears_metadata);
    RUN_TEST(test_set_result_end_to_end_through_worker_pool);
    RUN_TEST(test_set_result_strings_survive_lua_gc);
    RUN_TEST(test_workflow_job_with_intentional_failure);
    RUN_TEST(test_notify_send_returns_error_handle);
    RUN_TEST(test_mail_send_returns_error_handle);
    RUN_TEST(test_notify_send_sync_returns_nil_error);
    RUN_TEST(test_mail_send_sync_returns_nil_error);
    RUN_TEST(test_scoreboard_json_emits_current_state);

    return UNITY_END();
}