/*
 * Mock Scripting Scoreboard JSON functions for unit testing
 *
 * This file provides mock implementations of scripting_scoreboard_snapshot_json
 * and scripting_free_job_list to enable unit testing of code that depends on
 * the scoreboard JSON module without requiring the full scripting subsystem.
 */

#ifndef MOCK_SCOREBOARD_JSON_H
#define MOCK_SCOREBOARD_JSON_H

#include <stdbool.h>
#include <stddef.h>
#include <jansson.h>
#include <src/scripting/scoreboard_json.h>

// Mock function declarations
#ifdef USE_MOCK_SCOREBOARD_JSON
#define scripting_scoreboard_snapshot_json mock_scripting_scoreboard_snapshot_json
#define scripting_free_job_list mock_scripting_free_job_list
#endif

// Mock implementations
json_t* mock_scripting_scoreboard_snapshot_json(size_t max_jobs, bool include_params_json);
void mock_scripting_free_job_list(json_t* jobs_array);

// Mock control functions
void mock_scoreboard_json_reset_all(void);
void mock_scoreboard_json_set_snapshot_result(json_t* result);

#endif /* MOCK_SCOREBOARD_JSON_H */
