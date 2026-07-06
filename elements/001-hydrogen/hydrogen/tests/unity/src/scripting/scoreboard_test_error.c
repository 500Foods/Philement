/*
 * Unity Test File: scoreboard_test_error.c
 *
 * Phase 24 of the LUA_PLAN. Tests the scoreboard_update_error function
 * and error field handling.
 *
 * Validates:
 *   - scoreboard_update_error stores error_message and error_traceback
 *   - scoreboard_find copies error fields correctly
 *   - scoreboard_snapshot_json includes error fields for failed jobs
 *   - error fields are NULL when job succeeds
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/scripting/scoreboard.h>
#include <src/scripting/scripting.h>
#include <src/scripting/scoreboard_json.h>
#include <src/scripting/lua_context.h>

static AppConfig mock_app_config_storage = {0};

void test_scoreboard_update_error_stores_fields(void);
void test_scoreboard_update_error_clears_on_success(void);
void test_scoreboard_find_copies_error_fields(void);
void test_scoreboard_snapshot_includes_error_fields(void);
void test_scoreboard_snapshot_missing_error_fields_for_success(void);
void test_scoreboard_update_error_populated_with_real_traceback(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    app_config = NULL;
}

void test_scoreboard_update_error_stores_fields(void) {
    app_config->scripting.Enabled = true;
    scripting_init_state();
    scripting_scoreboard = scoreboard_create();
    TEST_ASSERT_NOT_NULL(scripting_scoreboard);

    char* job_id = scoreboard_submit(scripting_scoreboard, "test_script", NULL);
    TEST_ASSERT_NOT_NULL(job_id);

    bool updated = scoreboard_update_error(scripting_scoreboard, job_id,
                                            "attempt to index a nil value",
                                            "[1] main:1: attempt to index a nil value\n[2] scheduler:5: in function 'process'");
    TEST_ASSERT_TRUE(updated);

    ScoreboardEntry* entry = scoreboard_find(scripting_scoreboard, job_id);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_STRING("attempt to index a nil value", entry->error_message);
    TEST_ASSERT_NOT_NULL(entry->error_traceback);
    TEST_ASSERT_NOT_NULL(strstr(entry->error_traceback, "[1] main:1:"));
    TEST_ASSERT_NOT_NULL(strstr(entry->error_traceback, "[2] scheduler:5:"));

    scoreboard_entry_free(entry);
    free(job_id);
    scoreboard_destroy(scripting_scoreboard);
    scripting_scoreboard = NULL;
    scripting_cleanup_state();
}

void test_scoreboard_update_error_populated_with_real_traceback(void) {
    app_config->scripting.Enabled = true;
    scripting_init_state();
    scripting_scoreboard = scoreboard_create();
    TEST_ASSERT_NOT_NULL(scripting_scoreboard);

    char* job_id = scoreboard_submit(scripting_scoreboard, "test_script", NULL);
    TEST_ASSERT_NOT_NULL(job_id);

    lua_State* L = H_lua_create_context();
    TEST_ASSERT_NOT_NULL(L);

    const char* code = "error('phase24 error')";
    int rc = H_lua_run_string(L, code, "[test:real]");
    TEST_ASSERT_EQUAL_INT(LUA_ERRRUN, rc);

    const char* lua_err = lua_tostring(L, -1);
    TEST_ASSERT_NOT_NULL(lua_err);

    char* traceback = H_lua_build_traceback(L);
    TEST_ASSERT_NOT_NULL(traceback);

    bool updated = scoreboard_update_error(scripting_scoreboard, job_id,
                                            lua_err, traceback);
    TEST_ASSERT_TRUE(updated);

    ScoreboardEntry* entry = scoreboard_find(scripting_scoreboard, job_id);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_NOT_NULL(entry->error_message);
    TEST_ASSERT_TRUE(strlen(entry->error_message) > 0);
    TEST_ASSERT_NOT_NULL(entry->error_traceback);
    TEST_ASSERT_TRUE(strlen(entry->error_traceback) > 0);

    free(traceback);
    lua_pop(L, 1);
    H_lua_destroy_context(L);

    scoreboard_entry_free(entry);
    free(job_id);
    scoreboard_destroy(scripting_scoreboard);
    scripting_scoreboard = NULL;
    scripting_cleanup_state();
}

void test_scoreboard_update_error_clears_on_success(void) {
    app_config->scripting.Enabled = true;
    scripting_init_state();
    scripting_scoreboard = scoreboard_create();
    TEST_ASSERT_NOT_NULL(scripting_scoreboard);

    char* job_id = scoreboard_submit(scripting_scoreboard, "test_script", NULL);
    TEST_ASSERT_NOT_NULL(job_id);

    // Set error first
    bool updated = scoreboard_update_error(scripting_scoreboard, job_id,
                                            "error message", "traceback");
    TEST_ASSERT_TRUE(updated);

    // Now clear by passing empty strings
    updated = scoreboard_update_error(scripting_scoreboard, job_id, "", "");
    TEST_ASSERT_TRUE(updated);

    ScoreboardEntry* entry = scoreboard_find(scripting_scoreboard, job_id);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_NULL(entry->error_message);
    TEST_ASSERT_NULL(entry->error_traceback);

    scoreboard_entry_free(entry);
    free(job_id);
    scoreboard_destroy(scripting_scoreboard);
    scripting_scoreboard = NULL;
    scripting_cleanup_state();
}

void test_scoreboard_find_copies_error_fields(void) {
    app_config->scripting.Enabled = true;
    scripting_init_state();
    scripting_scoreboard = scoreboard_create();
    TEST_ASSERT_NOT_NULL(scripting_scoreboard);

    char* job_id = scoreboard_submit(scripting_scoreboard, "test_script", NULL);
    TEST_ASSERT_NOT_NULL(job_id);

    scoreboard_update_error(scripting_scoreboard, job_id,
                             "test error", "test traceback");

    // Find should return a copy
    ScoreboardEntry* entry1 = scoreboard_find(scripting_scoreboard, job_id);
    TEST_ASSERT_NOT_NULL(entry1);
    TEST_ASSERT_NOT_NULL(entry1->error_message);
    TEST_ASSERT_EQUAL_STRING("test error", entry1->error_message);

    // Modify the original entry's error (simulating concurrent update)
    scoreboard_update_error(scripting_scoreboard, job_id, "new error", "new traceback");

    // entry1 should still have the old values (it's a copy)
    TEST_ASSERT_EQUAL_STRING("test error", entry1->error_message);

    scoreboard_entry_free(entry1);

    // Find again should get the new values
    ScoreboardEntry* entry2 = scoreboard_find(scripting_scoreboard, job_id);
    TEST_ASSERT_NOT_NULL(entry2);
    TEST_ASSERT_EQUAL_STRING("new error", entry2->error_message);

    scoreboard_entry_free(entry2);
    free(job_id);
    scoreboard_destroy(scripting_scoreboard);
    scripting_scoreboard = NULL;
    scripting_cleanup_state();
}

void test_scoreboard_snapshot_includes_error_fields(void) {
    app_config->scripting.Enabled = true;
    scripting_init_state();
    scripting_scoreboard = scoreboard_create();
    TEST_ASSERT_NOT_NULL(scripting_scoreboard);

    char* job_id = scoreboard_submit(scripting_scoreboard, "test_script", NULL);
    TEST_ASSERT_NOT_NULL(job_id);

    scoreboard_update_status(scripting_scoreboard, job_id, SCOREBOARD_JOB_FAILED);
    scoreboard_update_error(scripting_scoreboard, job_id,
                             "runtime error: division by zero",
                             "[1] main:10: division by zero\n[2] util.lua:5: in function 'divide'");

    json_t* snapshot = scripting_scoreboard_snapshot_json(100, false);
    TEST_ASSERT_NOT_NULL(snapshot);

    json_t* jobs = json_object_get(snapshot, "jobs");
    TEST_ASSERT_NOT_NULL(jobs);
    TEST_ASSERT_EQUAL_INT(1, json_array_size(jobs));

    json_t* job = json_array_get(jobs, 0);
    TEST_ASSERT_NOT_NULL(job);

    json_t* error_msg = json_object_get(job, "error_message");
    TEST_ASSERT_NOT_NULL(error_msg);
    TEST_ASSERT_TRUE(json_is_string(error_msg));
    TEST_ASSERT_EQUAL_STRING("runtime error: division by zero", json_string_value(error_msg));

    json_t* error_tb = json_object_get(job, "error_traceback");
    TEST_ASSERT_NOT_NULL(error_tb);
    TEST_ASSERT_TRUE(json_is_string(error_tb));
    TEST_ASSERT_NOT_NULL(strstr(json_string_value(error_tb), "main:10:"));

    json_decref(snapshot);
    free(job_id);
    scoreboard_destroy(scripting_scoreboard);
    scripting_scoreboard = NULL;
    scripting_cleanup_state();
}

void test_scoreboard_snapshot_missing_error_fields_for_success(void) {
    app_config->scripting.Enabled = true;
    scripting_init_state();
    scripting_scoreboard = scoreboard_create();
    TEST_ASSERT_NOT_NULL(scripting_scoreboard);

    char* job_id = scoreboard_submit(scripting_scoreboard, "test_script", NULL);
    TEST_ASSERT_NOT_NULL(job_id);

    // Job succeeds, no error fields
    scoreboard_update_status(scripting_scoreboard, job_id, SCOREBOARD_JOB_COMPLETED);

    json_t* snapshot = scripting_scoreboard_snapshot_json(100, false);
    TEST_ASSERT_NOT_NULL(snapshot);

    json_t* jobs = json_object_get(snapshot, "jobs");
    json_t* job = json_array_get(jobs, 0);

    json_t* error_msg = json_object_get(job, "error_message");
    TEST_ASSERT_NULL(error_msg);

    json_t* error_tb = json_object_get(job, "error_traceback");
    TEST_ASSERT_NULL(error_tb);

    json_decref(snapshot);
    free(job_id);
    scoreboard_destroy(scripting_scoreboard);
    scripting_scoreboard = NULL;
    scripting_cleanup_state();
}

void setUp(void);
void tearDown(void);

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_scoreboard_update_error_stores_fields);
    RUN_TEST(test_scoreboard_update_error_clears_on_success);
    RUN_TEST(test_scoreboard_find_copies_error_fields);
    RUN_TEST(test_scoreboard_snapshot_includes_error_fields);
    RUN_TEST(test_scoreboard_snapshot_missing_error_fields_for_success);
    RUN_TEST(test_scoreboard_update_error_populated_with_real_traceback);

    return UNITY_END();
}