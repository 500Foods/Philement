/*
 * Unity Test File: scoreboard_json_test.c
 *
 * Phase 23 of the LUA_PLAN. Tests the scoreboard_json module that
 * provides JSON snapshots of the scoreboard for the system status
 * endpoint.
 *
 * Validates:
 *   - scripting_collect_metrics populates counts correctly
 *   - scripting_scoreboard_snapshot_json builds valid JSON
 *   - Empty scoreboard returns empty jobs array
 *   - Populated scoreboard includes job details
 *   - Privacy filtering excludes params_json by default
 *   - max_jobs parameter limits output size
 *   - Memory cleanup is correct
 */

 // Project header + Unity
#include <src/hydrogen.h>
#include <unity.h>

// Module under test
#include <src/scripting/scoreboard.h>
#include <src/scripting/scripting.h>
#include <src/scripting/scoreboard_json.h>

// Mock app_config
static AppConfig mock_app_config_storage = {0};

// Forward declarations
void test_collect_metrics_disabled(void);
void test_collect_metrics_enabled_empty(void);
void test_scoreboard_snapshot_empty(void);
void test_scoreboard_snapshot_with_jobs(void);
void test_scoreboard_snapshot_privacy_filter(void);
void test_scoreboard_snapshot_max_jobs_limit(void);
void test_scoreboard_snapshot_includes_all_fields(void);
void test_scripting_metrics_structure(void);
void test_scoreboard_snapshot_includes_result_fields(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    app_config = NULL;
}

// Test that metrics collection returns disabled when scripting is off
void test_collect_metrics_disabled(void) {
    app_config->scripting.Enabled = false;

    ScriptingMetrics metrics = scripting_get_metrics();

    TEST_ASSERT_FALSE(metrics.enabled);
    TEST_ASSERT_EQUAL(0, metrics.worker_threads);
    TEST_ASSERT_EQUAL(0, metrics.http_worker_threads);
    TEST_ASSERT_EQUAL(0, metrics.total_jobs);
    TEST_ASSERT_EQUAL(0, metrics.pending_jobs);
    TEST_ASSERT_EQUAL(0, metrics.running_jobs);
    TEST_ASSERT_EQUAL(0, metrics.completed_jobs);
    TEST_ASSERT_EQUAL(0, metrics.failed_jobs);
    TEST_ASSERT_EQUAL(0, metrics.killed_jobs);
}

// Test metrics collection when enabled but scoreboard is empty
void test_collect_metrics_enabled_empty(void) {
    app_config->scripting.Enabled = true;
    app_config->scripting.WorkerCount = 2;
    app_config->scripting.HTTPWorkerCount = 4;

    // Initialize scripting subsystem state (without starting workers)
    scripting_init_state();

    ScriptingMetrics metrics = scripting_get_metrics();

    TEST_ASSERT_TRUE(metrics.enabled);
    // Workers not started, so thread count is 0
    TEST_ASSERT_EQUAL(0, metrics.worker_threads);
    TEST_ASSERT_EQUAL(0, metrics.http_worker_threads);
    TEST_ASSERT_EQUAL(0, metrics.total_jobs);

    scripting_cleanup_state();
}

// Test empty scoreboard snapshot
void test_scoreboard_snapshot_empty(void) {
    json_t* snapshot = scripting_scoreboard_snapshot_json(100, false);

    TEST_ASSERT_NOT_NULL(snapshot);
    TEST_ASSERT_TRUE(json_is_object(snapshot));

    json_t* enabled = json_object_get(snapshot, "enabled");
    TEST_ASSERT_NOT_NULL(enabled);
    TEST_ASSERT_TRUE(json_is_boolean(enabled));

    json_t* worker_threads = json_object_get(snapshot, "worker_threads");
    TEST_ASSERT_NOT_NULL(worker_threads);
    TEST_ASSERT_TRUE(json_is_integer(worker_threads));

    json_t* http_worker_threads = json_object_get(snapshot, "http_worker_threads");
    TEST_ASSERT_NOT_NULL(http_worker_threads);
    TEST_ASSERT_TRUE(json_is_integer(http_worker_threads));

    json_t* total = json_object_get(snapshot, "total_jobs");
    TEST_ASSERT_NOT_NULL(total);
    TEST_ASSERT_TRUE(json_is_integer(total));
    TEST_ASSERT_EQUAL_INT(0, json_integer_value(total));

    json_t* jobs = json_object_get(snapshot, "jobs");
    TEST_ASSERT_NOT_NULL(jobs);
    TEST_ASSERT_TRUE(json_is_array(jobs));
    TEST_ASSERT_EQUAL_INT(0, json_array_size(jobs));

    json_decref(snapshot);
}

// Test snapshot with populated scoreboard
void test_scoreboard_snapshot_with_jobs(void) {
    // Enable scripting in config
    app_config->scripting.Enabled = true;

    // Initialize subsystem and create scoreboard
    scripting_init_state();
    scripting_scoreboard = scoreboard_create();
    TEST_ASSERT_NOT_NULL(scripting_scoreboard);

    // Submit a job
    char* job_id = scoreboard_submit(scripting_scoreboard, "test_script", "{\"param\": \"value\"}");
    TEST_ASSERT_NOT_NULL(job_id);

    // Take snapshot
    json_t* snapshot = scripting_scoreboard_snapshot_json(100, false);
    TEST_ASSERT_NOT_NULL(snapshot);

    json_t* total = json_object_get(snapshot, "total_jobs");
    TEST_ASSERT_NOT_NULL(total);
    TEST_ASSERT_EQUAL_INT(1, json_integer_value(total));

    json_t* pending = json_object_get(snapshot, "pending_jobs");
    TEST_ASSERT_NOT_NULL(pending);
    TEST_ASSERT_EQUAL_INT(1, json_integer_value(pending));

    json_t* jobs = json_object_get(snapshot, "jobs");
    TEST_ASSERT_NOT_NULL(jobs);
    TEST_ASSERT_TRUE(json_is_array(jobs));
    TEST_ASSERT_EQUAL_INT(1, json_array_size(jobs));

    json_t* job = json_array_get(jobs, 0);
    TEST_ASSERT_NOT_NULL(job);
    TEST_ASSERT_TRUE(json_is_object(job));

    json_t* job_id_json = json_object_get(job, "job_id");
    TEST_ASSERT_NOT_NULL(job_id_json);
    TEST_ASSERT_TRUE(json_is_string(job_id_json));
    TEST_ASSERT_EQUAL_STRING(job_id, json_string_value(job_id_json));

    json_t* script_name = json_object_get(job, "script_name");
    TEST_ASSERT_NOT_NULL(script_name);
    TEST_ASSERT_EQUAL_STRING("test_script", json_string_value(script_name));

    json_t* status = json_object_get(job, "status");
    TEST_ASSERT_NOT_NULL(status);
    TEST_ASSERT_EQUAL_STRING("pending", json_string_value(status));

    json_t* params_json = json_object_get(job, "params_json");
    // params_json should be NULL when privacy filter is active
    TEST_ASSERT_NULL(params_json);

    free(job_id);
    json_decref(snapshot);

    // Cleanup
    scoreboard_destroy(scripting_scoreboard);
    scripting_scoreboard = NULL;
    scripting_cleanup_state();
}

// Test privacy filtering excludes params_json
void test_scoreboard_snapshot_privacy_filter(void) {
    app_config->scripting.Enabled = true;
    scripting_init_state();
    scripting_scoreboard = scoreboard_create();
    TEST_ASSERT_NOT_NULL(scripting_scoreboard);

    char* job_id = scoreboard_submit(scripting_scoreboard, "secret_script", "{\"token\": \"abc123\"}");
    TEST_ASSERT_NOT_NULL(job_id);

    // With privacy filter (include_params_json = false)
    json_t* snapshot_filtered = scripting_scoreboard_snapshot_json(100, false);
    TEST_ASSERT_NOT_NULL(snapshot_filtered);

    json_t* jobs_filtered = json_object_get(snapshot_filtered, "jobs");
    json_t* job_filtered = json_array_get(jobs_filtered, 0);
    json_t* params_filtered = json_object_get(job_filtered, "params_json");
    TEST_ASSERT_NULL(params_filtered);

    // Without privacy filter (include_params_json = true)
    json_t* snapshot_full = scripting_scoreboard_snapshot_json(100, true);
    TEST_ASSERT_NOT_NULL(snapshot_full);

    json_t* jobs_full = json_object_get(snapshot_full, "jobs");
    json_t* job_full = json_array_get(jobs_full, 0);
    json_t* params_full = json_object_get(job_full, "params_json");
    TEST_ASSERT_NOT_NULL(params_full);
    TEST_ASSERT_TRUE(json_is_string(params_full));
    TEST_ASSERT_EQUAL_STRING("{\"token\": \"abc123\"}", json_string_value(params_full));

    free(job_id);
    json_decref(snapshot_filtered);
    json_decref(snapshot_full);

    scoreboard_destroy(scripting_scoreboard);
    scripting_scoreboard = NULL;
    scripting_cleanup_state();
}

// Test max_jobs parameter limits output
void test_scoreboard_snapshot_max_jobs_limit(void) {
    app_config->scripting.Enabled = true;
    scripting_init_state();
    scripting_scoreboard = scoreboard_create();
    TEST_ASSERT_NOT_NULL(scripting_scoreboard);

    // Submit 5 jobs
    for (int i = 0; i < 5; i++) {
        char* job_id = scoreboard_submit(scripting_scoreboard, "script", NULL);
        TEST_ASSERT_NOT_NULL(job_id);
        free(job_id);
    }

    // Request only 3 jobs
    json_t* snapshot = scripting_scoreboard_snapshot_json(3, false);
    TEST_ASSERT_NOT_NULL(snapshot);

    json_t* total = json_object_get(snapshot, "total_jobs");
    TEST_ASSERT_NOT_NULL(total);
    TEST_ASSERT_EQUAL_INT(5, json_integer_value(total));

    json_t* jobs = json_object_get(snapshot, "jobs");
    TEST_ASSERT_EQUAL_INT(3, json_array_size(jobs));

    json_decref(snapshot);

    scoreboard_destroy(scripting_scoreboard);
    scripting_scoreboard = NULL;
    scripting_cleanup_state();
}

// Test snapshot includes all expected fields
void test_scoreboard_snapshot_includes_all_fields(void) {
    json_t* snapshot = scripting_scoreboard_snapshot_json(100, false);
    TEST_ASSERT_NOT_NULL(snapshot);

    // Check top-level fields
    TEST_ASSERT_NOT_NULL(json_object_get(snapshot, "enabled"));
    TEST_ASSERT_NOT_NULL(json_object_get(snapshot, "worker_threads"));
    TEST_ASSERT_NOT_NULL(json_object_get(snapshot, "http_worker_threads"));
    TEST_ASSERT_NOT_NULL(json_object_get(snapshot, "total_jobs"));
    TEST_ASSERT_NOT_NULL(json_object_get(snapshot, "pending_jobs"));
    TEST_ASSERT_NOT_NULL(json_object_get(snapshot, "running_jobs"));
    TEST_ASSERT_NOT_NULL(json_object_get(snapshot, "completed_jobs"));
    TEST_ASSERT_NOT_NULL(json_object_get(snapshot, "failed_jobs"));
    TEST_ASSERT_NOT_NULL(json_object_get(snapshot, "killed_jobs"));
    TEST_ASSERT_NOT_NULL(json_object_get(snapshot, "jobs"));

    json_decref(snapshot);
}

// Test ScriptingMetrics structure is correctly populated
void test_scripting_metrics_structure(void) {
    app_config->scripting.Enabled = true;
    app_config->scripting.WorkerCount = 3;
    app_config->scripting.HTTPWorkerCount = 5;

    scripting_init_state();

    ScriptingMetrics metrics = scripting_get_metrics();

    TEST_ASSERT_TRUE(metrics.enabled);
    // Note: worker_threads and http_worker_threads will be 0 because
    // we haven't actually started the worker pools in this test
    TEST_ASSERT_EQUAL(0, metrics.worker_threads);
    TEST_ASSERT_EQUAL(0, metrics.http_worker_threads);

    scripting_cleanup_state();
}

void setUp(void);
void tearDown(void);

// Test that result_type and result_location appear in JSON output
void test_scoreboard_snapshot_includes_result_fields(void) {
    app_config->scripting.Enabled = true;
    scripting_init_state();
    scripting_scoreboard = scoreboard_create();
    TEST_ASSERT_NOT_NULL(scripting_scoreboard);

    char* job_id = scoreboard_submit(scripting_scoreboard, "result_test", NULL);
    TEST_ASSERT_NOT_NULL(job_id);

    // Set result metadata
    TEST_ASSERT_TRUE(scoreboard_update_result(scripting_scoreboard, job_id, "json", "/results/output.json"));

    // Take snapshot and verify result fields are present
    json_t* snapshot = scripting_scoreboard_snapshot_json(100, false);
    TEST_ASSERT_NOT_NULL(snapshot);

    json_t* jobs = json_object_get(snapshot, "jobs");
    TEST_ASSERT_NOT_NULL(jobs);
    TEST_ASSERT_EQUAL_INT(1, json_array_size(jobs));

    json_t* job = json_array_get(jobs, 0);
    TEST_ASSERT_NOT_NULL(job);

    json_t* result_type = json_object_get(job, "result_type");
    TEST_ASSERT_NOT_NULL(result_type);
    TEST_ASSERT_TRUE(json_is_string(result_type));
    TEST_ASSERT_EQUAL_STRING("json", json_string_value(result_type));

    json_t* result_location = json_object_get(job, "result_location");
    TEST_ASSERT_NOT_NULL(result_location);
    TEST_ASSERT_TRUE(json_is_string(result_location));
    TEST_ASSERT_EQUAL_STRING("/results/output.json", json_string_value(result_location));

    free(job_id);
    json_decref(snapshot);

    scoreboard_destroy(scripting_scoreboard);
    scripting_scoreboard = NULL;
    scripting_cleanup_state();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_collect_metrics_disabled);
    RUN_TEST(test_collect_metrics_enabled_empty);
    RUN_TEST(test_scoreboard_snapshot_empty);
    RUN_TEST(test_scoreboard_snapshot_with_jobs);
    RUN_TEST(test_scoreboard_snapshot_privacy_filter);
    RUN_TEST(test_scoreboard_snapshot_max_jobs_limit);
    RUN_TEST(test_scoreboard_snapshot_includes_all_fields);
    RUN_TEST(test_scripting_metrics_structure);
    RUN_TEST(test_scoreboard_snapshot_includes_result_fields);

    return UNITY_END();
}
