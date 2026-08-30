/*
 * Mock Scripting Scoreboard JSON functions for unit testing
 */

#include "mock_scoreboard_json.h"
#include <stdlib.h>

// Static state for mock control
static json_t* mock_snapshot_result = NULL;
static bool mock_snapshot_result_set = false;

/*
 * Mock implementation of scripting_scoreboard_snapshot_json
 */
__attribute__((weak))
json_t* mock_scripting_scoreboard_snapshot_json(size_t max_jobs, bool include_params_json) {
    (void)max_jobs;
    (void)include_params_json;

    if (mock_snapshot_result_set) {
        if (mock_snapshot_result) {
            return json_deep_copy(mock_snapshot_result);
        }
        return NULL;
    }

    // Default: return an empty snapshot object with empty jobs array
    json_t* snapshot = json_object();
    if (!snapshot) {
        return NULL;
    }
    json_object_set_new(snapshot, "jobs", json_array());
    return snapshot;
}

/*
 * Mock implementation of scripting_free_job_list
 */
__attribute__((weak))
void mock_scripting_free_job_list(json_t* jobs_array) {
    if (jobs_array) {
        json_decref(jobs_array);
    }
}

/*
 * Reset all mock state to defaults
 */
void mock_scoreboard_json_reset_all(void) {
    if (mock_snapshot_result) {
        json_decref(mock_snapshot_result);
        mock_snapshot_result = NULL;
    }
    mock_snapshot_result_set = false;
}

/*
 * Set the mock snapshot result (takes ownership)
 */
void mock_scoreboard_json_set_snapshot_result(json_t* result) {
    if (mock_snapshot_result) {
        json_decref(mock_snapshot_result);
    }
    mock_snapshot_result = result;
    mock_snapshot_result_set = true;
}
