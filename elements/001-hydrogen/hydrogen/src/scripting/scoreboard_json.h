/*
 * Scripting Subsystem - Scoreboard JSON Snapshot
 *
 * Phase 23: Provides a JSON snapshot of the scoreboard for the
 * system status endpoint. This is scripting-owned and called from
 * the status formatter, not from Lua.
 */

#ifndef HYDROGEN_SCRIPTING_SCOREBOARD_JSON_H
#define HYDROGEN_SCRIPTING_SCOREBOARD_JSON_H

#include <stdbool.h>
#include <jansson.h>
#include <src/globals.h>

struct ScriptingMetrics {
    bool enabled;
    int worker_threads;
    int http_worker_threads;
    int total_jobs;
    int pending_jobs;
    int running_jobs;
    int completed_jobs;
    int failed_jobs;
    int killed_jobs;
};

typedef struct ScriptingMetrics ScriptingMetrics;

void scripting_collect_metrics(ScriptingMetrics* metrics);

ScriptingMetrics scripting_get_metrics(void);

json_t* scripting_scoreboard_snapshot_json(size_t max_jobs, bool include_params_json);

void scripting_free_job_list(json_t* jobs_array);

#endif /* HYDROGEN_SCRIPTING_SCOREBOARD_JSON_H */