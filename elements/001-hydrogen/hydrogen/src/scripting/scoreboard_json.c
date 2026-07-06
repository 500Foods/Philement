/*
 * Scripting Subsystem - Scoreboard JSON Snapshot
 *
 * Phase 23: Provides a JSON snapshot of the scoreboard for the
 * system status endpoint. This is scripting-owned and called from
 * the status formatter, not from Lua.
 */

#include "scoreboard_json.h"
#include <src/hydrogen.h>
#include "scoreboard.h"
#include "scripting.h"
#include "worker_pool.h"
#include "http_pool.h"

static ScriptingMetrics cached_metrics = {0};

void scripting_collect_metrics(ScriptingMetrics* metrics) {
    if (!metrics) {
        return;
    }

    metrics->enabled = (app_config && app_config->scripting.Enabled);
    metrics->worker_threads = 0;
    metrics->http_worker_threads = 0;
    metrics->total_jobs = 0;
    metrics->pending_jobs = 0;
    metrics->running_jobs = 0;
    metrics->completed_jobs = 0;
    metrics->failed_jobs = 0;
    metrics->killed_jobs = 0;

    if (!metrics->enabled) {
        return;
    }

    if (scripting_threads.thread_count > 0) {
        metrics->worker_threads = scripting_threads.thread_count;
    }

    if (scripting_http_pool && scripting_http_pool->worker_count > 0) {
        metrics->http_worker_threads = scripting_http_pool->worker_count;
    }

    if (scripting_scoreboard) {
        ScoreboardEntry** list = NULL;
        size_t count = 0;
        if (scoreboard_list(scripting_scoreboard, &list, &count)) {
            metrics->total_jobs = (int)count;
            for (size_t i = 0; i < count; i++) {
                switch (list[i]->status) {
                    case SCOREBOARD_JOB_PENDING:
                        metrics->pending_jobs++;
                        break;
                    case SCOREBOARD_JOB_RUNNING:
                        metrics->running_jobs++;
                        break;
                    case SCOREBOARD_JOB_COMPLETED:
                        metrics->completed_jobs++;
                        break;
                    case SCOREBOARD_JOB_FAILED:
                        metrics->failed_jobs++;
                        break;
                    case SCOREBOARD_JOB_KILLED:
                        metrics->killed_jobs++;
                        break;
                }
            }
            scoreboard_list_free(list, count);
        }
    }
}

ScriptingMetrics scripting_get_metrics(void) {
    scripting_collect_metrics(&cached_metrics);
    return cached_metrics;
}

json_t* scripting_scoreboard_snapshot_json(size_t max_jobs, bool include_params_json) {
    json_t* result = json_object();
    if (!result) {
        return NULL;
    }

    ScriptingMetrics metrics = scripting_get_metrics();

    json_object_set_new(result, "enabled", json_boolean(metrics.enabled));
    json_object_set_new(result, "worker_threads", json_integer(metrics.worker_threads));
    json_object_set_new(result, "http_worker_threads", json_integer(metrics.http_worker_threads));

    json_object_set_new(result, "total_jobs", json_integer(metrics.total_jobs));
    json_object_set_new(result, "pending_jobs", json_integer(metrics.pending_jobs));
    json_object_set_new(result, "running_jobs", json_integer(metrics.running_jobs));
    json_object_set_new(result, "completed_jobs", json_integer(metrics.completed_jobs));
    json_object_set_new(result, "failed_jobs", json_integer(metrics.failed_jobs));
    json_object_set_new(result, "killed_jobs", json_integer(metrics.killed_jobs));

    json_t* jobs = json_array();
    if (!jobs) {
        json_decref(result);
        return NULL;
    }

    if (scripting_scoreboard) {
        ScoreboardEntry** list = NULL;
        size_t count = 0;
        if (scoreboard_list(scripting_scoreboard, &list, &count)) {
            size_t job_index = 0;
            for (size_t i = 0; i < count && job_index < max_jobs; i++) {
                ScoreboardEntry* entry = list[i];
                json_t* job_obj = json_object();
                if (!job_obj) {
                    scoreboard_list_free(list, count);
                    json_decref(jobs);
                    json_decref(result);
                    return NULL;
                }

                json_object_set_new(job_obj, "job_id", json_string(entry->job_id));
                json_object_set_new(job_obj, "script_name", json_string(entry->script_name ? entry->script_name : ""));

                const char* status_name = scoreboard_status_name(entry->status);
                json_object_set_new(job_obj, "status", json_string(status_name ? status_name : "unknown"));

                char time_buf[32];
                if (entry->created_at.tv_sec > 0) {
                    struct tm* tm_info = gmtime(&entry->created_at.tv_sec);
                    strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%SZ", tm_info);
                    json_object_set_new(job_obj, "created_at", json_string(time_buf));
                }
                if (entry->started_at.tv_sec > 0) {
                    struct tm* tm_info = gmtime(&entry->started_at.tv_sec);
                    strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%SZ", tm_info);
                    json_object_set_new(job_obj, "started_at", json_string(time_buf));
                }
                if (entry->finished_at.tv_sec > 0) {
                    struct tm* tm_info = gmtime(&entry->finished_at.tv_sec);
                    strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%SZ", tm_info);
                    json_object_set_new(job_obj, "finished_at", json_string(time_buf));
                }

                json_object_set_new(job_obj, "instruction_count", json_integer((json_int_t)entry->instruction_count));
                json_object_set_new(job_obj, "memory_used_kb", json_integer((json_int_t)entry->memory_used_kb));

                if (entry->current_state && entry->current_state[0] != '\0') {
                    json_object_set_new(job_obj, "current_state", json_string(entry->current_state));
                }

                json_object_set_new(job_obj, "max_runtime_seconds", json_integer(entry->max_runtime_seconds));
                json_object_set_new(job_obj, "kill_requested", json_boolean(entry->kill_requested));

                // Phase 24: include structured error info for failed/killed jobs.
                if (entry->error_message && entry->error_message[0] != '\0') {
                    json_object_set_new(job_obj, "error_message", json_string(entry->error_message));
                }
                if (entry->error_traceback && entry->error_traceback[0] != '\0') {
                    json_object_set_new(job_obj, "error_traceback", json_string(entry->error_traceback));
                }

                if (include_params_json && entry->params_json) {
                    json_object_set_new(job_obj, "params_json", json_string(entry->params_json));
                }

                // Phase 25: include artifact metadata when present.
                if (entry->result_type && entry->result_type[0] != '\0') {
                    json_object_set_new(job_obj, "result_type", json_string(entry->result_type));
                }
                if (entry->result_location && entry->result_location[0] != '\0') {
                    json_object_set_new(job_obj, "result_location", json_string(entry->result_location));
                }

                json_array_append_new(jobs, job_obj);
                job_index++;
            }
            scoreboard_list_free(list, count);
        }
    }

    json_object_set_new(result, "jobs", jobs);
    return result;
}

void scripting_free_job_list(json_t* jobs_array) {
    if (jobs_array) {
        json_decref(jobs_array);
    }
}