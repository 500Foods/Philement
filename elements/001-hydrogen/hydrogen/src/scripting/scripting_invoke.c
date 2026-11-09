/*
 * Scripting Subsystem - Client invoke submit (LUA_CLIENT Phase 2)
 */

#include <src/hydrogen.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "scripting_invoke.h"
#include "scripting.h"
#include "orchestrator.h"
#include "source_cache.h"
#include "worker_pool.h"
#include "script_registry.h"
#include "lua_context.h"
#include "scoreboard.h"
#include <string.h>

static scripting_invoke_load_source_fn g_load_source_hook = NULL;
static char scripting_wait_job_waiter_tag;

void scripting_invoke_set_load_source_hook(scripting_invoke_load_source_fn fn) {
    g_load_source_hook = fn;
}

const char* scripting_invoke_error_name(ScriptingInvokeError err) {
    switch (err) {
    case SCRIPTING_INVOKE_OK:
        return "ok";
    case SCRIPTING_INVOKE_ERR_DISABLED:
        return "scripting_disabled";
    case SCRIPTING_INVOKE_ERR_INVALID_NAME:
        return "invalid_script_name";
    case SCRIPTING_INVOKE_ERR_NO_DATABASE:
        return "no_database";
    case SCRIPTING_INVOKE_ERR_NOT_FOUND:
        return "script_not_found";
    case SCRIPTING_INVOKE_ERR_DB_TIMEOUT:
        return "db_timeout";
    case SCRIPTING_INVOKE_ERR_SUBMIT_FAILED:
        return "submit_failed";
    case SCRIPTING_INVOKE_ERR_INTERNAL:
    default:
        return "internal_error";
    }
}

bool scripting_invoke_parse_script_name(const char* script_name,
                                        char** group_out,
                                        char** script_out) {
    if (!script_name || !group_out || !script_out) {
        return false;
    }
    *group_out = NULL;
    *script_out = NULL;

    /* Reject slash form — Phase 0 canonical is Group.Name only. */
    if (strchr(script_name, '/') != NULL) {
        return false;
    }

    const char* dot = strchr(script_name, '.');
    if (!dot || dot == script_name || dot[1] == '\0') {
        return false;
    }
    /* Exactly one logical split: first dot; rest is script name (may contain dots). */
    size_t group_len = (size_t)(dot - script_name);
    char* group = malloc(group_len + 1);
    if (!group) {
        return false;
    }
    memcpy(group, script_name, group_len);
    group[group_len] = '\0';

    char* script = strdup(dot + 1);
    if (!script) {
        free(group);
        return false;
    }

    *group_out = group;
    *script_out = script;
    return true;
}

char* scripting_invoke_load_source(const char* group_name,
                                   const char* script_name,
                                   int fetch_timeout_seconds,
                                   ScriptingInvokeError* err_out) {
    if (err_out) {
        *err_out = SCRIPTING_INVOKE_ERR_INTERNAL;
    }
    if (!group_name || !script_name || fetch_timeout_seconds <= 0) {
        if (err_out) {
            *err_out = SCRIPTING_INVOKE_ERR_INTERNAL;
        }
        return NULL;
    }

    if (g_load_source_hook) {
        return g_load_source_hook(group_name, script_name,
                                  fetch_timeout_seconds, err_out);
    }

    /*
     * Client REST path (LUA_CLIENT Phase 7): always hit QueryRef #149
     * (invokable only). Do not use source_cache for the allowlist
     * decision — a prior require()/Orchestrator load of a non-invokable
     * script must not make it REST-callable.
     */
    const char* database = orchestrator_resolve_database();
    if (!database || database[0] == '\0') {
        if (err_out) {
            *err_out = SCRIPTING_INVOKE_ERR_NO_DATABASE;
        }
        return NULL;
    }

    char* fetched = scripting_fetch_invokable_script_source(
        group_name, script_name, database, fetch_timeout_seconds);
    if (!fetched) {
        /* Missing, non-invokable, timeout, or QTC miss → same not_found
         * (Phase 0 existence-hiding 404). */
        if (err_out) {
            *err_out = SCRIPTING_INVOKE_ERR_NOT_FOUND;
        }
        return NULL;
    }
    if (fetched[0] == '\0') {
        free(fetched);
        if (err_out) {
            *err_out = SCRIPTING_INVOKE_ERR_NOT_FOUND;
        }
        return NULL;
    }

    if (scripting_source_cache) {
        (void)source_cache_put(scripting_source_cache, group_name, script_name,
                               fetched);
    }

    if (err_out) {
        *err_out = SCRIPTING_INVOKE_OK;
    }
    return fetched;
}

ScriptingInvokeError scripting_submit_job_from_db(
    const char* script_name,
    const char* params_json,
    const ScoreboardJobLimits* limits,
    int fetch_timeout_seconds,
    char** job_id_out) {
    if (job_id_out) {
        *job_id_out = NULL;
    }

    if (!script_name || !job_id_out) {
        return SCRIPTING_INVOKE_ERR_INTERNAL;
    }

    if (!app_config || !app_config->scripting.Enabled) {
        return SCRIPTING_INVOKE_ERR_DISABLED;
    }
    if (!scripting_workers || !scripting_scoreboard) {
        return SCRIPTING_INVOKE_ERR_DISABLED;
    }

    char* group = NULL;
    char* script = NULL;
    if (!scripting_invoke_parse_script_name(script_name, &group, &script)) {
        return SCRIPTING_INVOKE_ERR_INVALID_NAME;
    }

    ScriptingInvokeError load_err = SCRIPTING_INVOKE_ERR_INTERNAL;
    char* options_json = NULL;
    const char* database = orchestrator_resolve_database();
    char* source = NULL;
    if (!database) {
        free(group);
        free(script);
        return SCRIPTING_INVOKE_ERR_NO_DATABASE;
    }
    source = scripting_fetch_invokable_script_source_with_options(
        group, script, database, fetch_timeout_seconds, &options_json);
    free(group);
    free(script);
    if (!source) {
        free(options_json);
        return SCRIPTING_INVOKE_ERR_NOT_FOUND;
    }
    (void)load_err;

    ScoreboardJobLimits merged;
    memset(&merged, 0, sizeof(merged));
    merged.enforce_limits = true;
    H_LuaScriptOptions opt;
    H_lua_script_options_clear(&opt);
    if (options_json && !H_lua_parse_script_options(options_json, &opt)) {
        log_this(SR_SCRIPTING,
                 "scripts.options malformed for %s; ignoring",
                 LOG_LEVEL_ALERT, 1, script_name);
        H_lua_script_options_clear(&opt);
    }
    /* Precedence: script options first, then explicit submit-time
     * overrides, then scoreboard resolves zeros to config defaults.
     * Memory is clamped to config MemoryHardLimitKB. */
    H_lua_limits_from_script_options(&opt, &merged);
    if (limits) {
        if (limits->instruction_hook_interval > 0) {
            merged.instruction_hook_interval = limits->instruction_hook_interval;
        }
        if (limits->memory_soft_limit_kb > 0) {
            merged.memory_soft_limit_kb = limits->memory_soft_limit_kb;
        }
        if (limits->memory_hard_limit_kb > 0) {
            merged.memory_hard_limit_kb = limits->memory_hard_limit_kb;
        }
        if (limits->max_runtime_seconds > 0) {
            merged.max_runtime_seconds = limits->max_runtime_seconds;
        }
        merged.enforce_limits = limits->enforce_limits;
        /* Clamp again after submit override. */
        {
            H_LuaScriptOptions clamp_only;
            H_lua_script_options_clear(&clamp_only);
            H_lua_limits_from_script_options(&clamp_only, &merged);
        }
    }

    char* job_id = scripting_submit_job_with_source_and_limits(
        script_name, source, params_json, &merged);
    free(source);
    if (job_id && options_json && scripting_scoreboard) {
        (void)scoreboard_set_options_json(scripting_scoreboard, job_id,
                                          options_json);
    }
    free(options_json);

    if (!job_id) {
        return SCRIPTING_INVOKE_ERR_SUBMIT_FAILED;
    }

    *job_id_out = job_id;
    return SCRIPTING_INVOKE_OK;
}

const char* scripting_wait_result_name(ScriptingWaitResult r) {
    switch (r) {
    case SCRIPTING_WAIT_COMPLETED:
        return "completed";
    case SCRIPTING_WAIT_FAILED:
        return "failed";
    case SCRIPTING_WAIT_KILLED:
        return "killed";
    case SCRIPTING_WAIT_TIMEOUT:
        return "timeout";
    case SCRIPTING_WAIT_NOT_FOUND:
        return "not_found";
    case SCRIPTING_WAIT_SHUTDOWN:
        return "shutdown";
    case SCRIPTING_WAIT_INTERNAL:
    default:
        return "internal_error";
    }
}

ScriptingWaitResult scripting_wait_result_from_status(ScoreboardJobStatus st) {
    switch (st) {
    case SCOREBOARD_JOB_COMPLETED:
        return SCRIPTING_WAIT_COMPLETED;
    case SCOREBOARD_JOB_FAILED:
        return SCRIPTING_WAIT_FAILED;
    case SCOREBOARD_JOB_KILLED:
        return SCRIPTING_WAIT_KILLED;
    case SCOREBOARD_JOB_PENDING:
    case SCOREBOARD_JOB_RUNNING:
        return SCRIPTING_WAIT_INTERNAL;
    default:
        return SCRIPTING_WAIT_INTERNAL;
    }
}

bool scripting_wait_status_is_terminal(ScoreboardJobStatus st) {
    return st == SCOREBOARD_JOB_COMPLETED
        || st == SCOREBOARD_JOB_FAILED
        || st == SCOREBOARD_JOB_KILLED;
}

ScriptingWaitResult scripting_wait_job(const char* job_id,
                                       int timeout_seconds,
                                       ScoreboardEntry** out_entry) {
    if (out_entry) {
        *out_entry = NULL;
    }
    if (!job_id || job_id[0] == '\0' || timeout_seconds <= 0) {
        return SCRIPTING_WAIT_INTERNAL;
    }
    if (!scripting_scoreboard) {
        return SCRIPTING_WAIT_INTERNAL;
    }

    struct timespec start;
    ScriptingWaitResult wr = SCRIPTING_WAIT_INTERNAL;
    bool attached_by_us = false;
    bool has_waiter = false;
    void* waiter_handle = NULL;

    if (!scoreboard_attach_waiter(scripting_scoreboard, job_id,
                                  &scripting_wait_job_waiter_tag, NULL)) {
        return SCRIPTING_WAIT_NOT_FOUND;
    }
    if (scoreboard_get_waiter(scripting_scoreboard, job_id, &has_waiter,
                              &waiter_handle, NULL)
        && has_waiter
        && waiter_handle == &scripting_wait_job_waiter_tag) {
        attached_by_us = true;
    }

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (;;) {
        ScoreboardEntry* e = scoreboard_find(scripting_scoreboard, job_id);
        if (!e) {
            wr = SCRIPTING_WAIT_NOT_FOUND;
            break;
        }

        if (scripting_wait_status_is_terminal(e->status)) {
            wr = scripting_wait_result_from_status(e->status);
            if (out_entry) {
                *out_entry = e;
            } else {
                scoreboard_entry_free(e);
            }
            break;
        }
        scoreboard_entry_free(e);

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000L
                        + (now.tv_nsec - start.tv_nsec) / 1000000L;
        long budget_ms = (long)timeout_seconds * 1000L;
        if (elapsed_ms >= budget_ms) {
            (void)scoreboard_request_kill(scripting_scoreboard, job_id);
            if (out_entry) {
                *out_entry = scoreboard_find(scripting_scoreboard, job_id);
            }
            wr = SCRIPTING_WAIT_TIMEOUT;
            break;
        }

        if (scripting_system_shutdown != 0) {
            if (out_entry) {
                *out_entry = scoreboard_find(scripting_scoreboard, job_id);
            }
            wr = SCRIPTING_WAIT_SHUTDOWN;
            break;
        }
        usleep(10000);
    }

    if (attached_by_us) {
        (void)scoreboard_clear_waiter(scripting_scoreboard, job_id);
    }
    return wr;
}
