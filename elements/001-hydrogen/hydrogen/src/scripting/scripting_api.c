/*
 * Scripting Subsystem - Host API Implementation
 *
 * Phase 6 of the LUA_PLAN. C-side implementations of the H.log and
 * H.system functions exposed to Lua scripts.
 *
 *   H.log.{trace, debug, info, warn, error, fatal}(fmt, ...)
 *   H.system.{uptime, now, now_iso, instance_id, version}()
 *
 * Design notes:
 *
 *   H.log.* uses Lua's own string.format to build the message, then
 *   forwards the result to log_this. This delegates all format
 *   specifier support to Lua and keeps our C side small. The format
 *   specifier count is checked against the arg count and a clear
 *   error is logged (without raising) if they disagree - the host
 *   log path should never crash the host.
 *
 *   H.system.* are all O(1) and cannot fail. instance_id lazily
 *   generates a UUID on first call and caches it for the process
 *   lifetime; subsequent calls return the cached value.
 */

 // Project includes
#include <src/hydrogen.h>

// System includes
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

// Third-party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <jansson.h>

// Local includes
#include "scripting_api.h"
#include "lua_context.h"   // H_lua_job_context, H_lua_get_job_context
#include "scoreboard.h"     // scoreboard_update_current_state
#include "scripting.h"      // scripting_system_shutdown, scripting_scoreboard
#include "scripting_handle.h" // H_Handle, H_Handle_new/free/check
#include "source_cache.h"   // source_cache_get/put
#include "orchestrator.h"   // scripting_fetch_script_source
#include "worker_pool.h"    // scripting_submit_job_with_source

// Phase 13 includes
#include <src/api/conduit/conduit_helpers.h>   // generate_query_id
#include <src/api/auth/auth_service.h>         // validate_jwt, jwt_claims_t

// Phase 16 includes
#include <curl/curl.h>                         // curl_slist, curl_slist_append/free_all
#include "http_client.h"                       // scripting_http_get/_post
#include "http_pool.h"                         // scripting_http_pool_submit (Phase 17)
#include "src/api/auth/oidc_rp/oidc_rp_http.h" // OidcRpHttpResponse, oidc_rp_http_response_free

// H.log.* implementations //////////////////////////////////////////////////

/*
 * H.log.<level>(fmt, ...) - format fmt with the trailing args and emit
 * a log line at the given priority, tagged with the Scripting subsystem.
 *
 * Lua-side signature: (format_string, ...args)
 * C-side call:        lua_pushcfunction -> H_lua_log_<LEVEL>(L)
 *
 * On success, the formatted message is forwarded to log_this and
 * nothing is left on the Lua stack. On argument-count mismatch, a
 * clear error is logged and nothing is raised - the host log path
 * is a leaf, not a source of crashes.
 */
static int H_lua_log_at_level(lua_State* L, int priority) {
    // Argument validation: at least a format string is required.
    int nargs = lua_gettop(L);
    if (nargs < 1) {
        log_this(SR_SCRIPTING, "H.log: missing format string", LOG_LEVEL_ERROR, 0);
        return 0;
    }

    if (!lua_isstring(L, 1)) {
        // First arg must be a string. lua_tostring raises a controlled
        // error in some cases, so we check explicitly and bail with a
        // log line rather than a Lua error.
        log_this(SR_SCRIPTING, "H.log: first argument must be a string", LOG_LEVEL_ERROR, 0);
        return 0;
    }

    const char* fmt = lua_tostring(L, 1);
    size_t expected = count_format_specifiers(fmt);
    size_t actual = (size_t)(nargs - 1);

    if (expected != actual) {
        log_this(SR_SCRIPTING,
                 "H.log: format/args mismatch (format expects %zu, got %zu)",
                 LOG_LEVEL_ERROR, 2, expected, actual);
        return 0;
    }

    // Build the formatted message via Lua's string.format.
    // Stack layout before: [fmt, arg1, arg2, ...]
    lua_getglobal(L, "string");
    if (!lua_istable(L, -1)) {
        log_this(SR_SCRIPTING, "H.log: string table not available", LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return 0;
    }
    lua_getfield(L, -1, "format");
    if (!lua_isfunction(L, -1)) {
        log_this(SR_SCRIPTING, "H.log: string.format not available", LOG_LEVEL_ERROR, 0);
        lua_pop(L, 2);
        return 0;
    }
    lua_remove(L, -2); // pop the string table; format function is now on top

    // Re-push fmt and all args in order, leaving them for string.format.
    // Originals stay on the stack; copies become format's args.
    for (int i = 1; i <= nargs; i++) {
        lua_pushvalue(L, i);
    }

    // string.format(fmt, arg1, ..., argN) -> 1 result
    if (lua_pcall(L, nargs, 1, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        log_this(SR_SCRIPTING, "H.log: string.format failed: %s",
                 LOG_LEVEL_ERROR, 1, err ? err : "(no message)");
        lua_pop(L, 1); // pop the error
        return 0;
    }

    // lua_pcall left one result (the formatted string). Forward it
    // to log_this with num_args=0 (the message is already formatted).
    const char* msg = lua_tostring(L, -1);
    if (msg) {
        log_this(SR_SCRIPTING, msg, priority, 0);
    } else {
        log_this(SR_SCRIPTING, "H.log: formatted message is not a string",
                 LOG_LEVEL_ERROR, 0);
    }
    lua_pop(L, 1); // pop the formatted message

    return 0;
}

static int H_lua_log_trace(lua_State* L) { return H_lua_log_at_level(L, LOG_LEVEL_TRACE); }
static int H_lua_log_debug(lua_State* L) { return H_lua_log_at_level(L, LOG_LEVEL_DEBUG); }
static int H_lua_log_info(lua_State* L)  { return H_lua_log_at_level(L, LOG_LEVEL_STATE); }
static int H_lua_log_warn(lua_State* L)  { return H_lua_log_at_level(L, LOG_LEVEL_ALERT); }
static int H_lua_log_error(lua_State* L) { return H_lua_log_at_level(L, LOG_LEVEL_ERROR); }
static int H_lua_log_fatal(lua_State* L) { return H_lua_log_at_level(L, LOG_LEVEL_FATAL); }

// H.system.* implementations ///////////////////////////////////////////////

/*
 * H.system.uptime() - seconds since server start.
 *
 * Uses get_server_start_time() from utils_time. If the start time has
 * not been recorded (e.g. in a Unity test that doesn't initialize the
 * time subsystem), returns 0 rather than producing a negative number.
 */
static int H_lua_system_uptime(lua_State* L) {
    time_t start = get_server_start_time();
    time_t now = time(NULL);
    lua_Number elapsed = (start == 0) ? 0.0 : (lua_Number)(now - start);
    lua_pushnumber(L, elapsed);
    return 1;
}

/*
 * H.system.now() - epoch seconds as a float.
 */
static int H_lua_system_now(lua_State* L) {
    lua_pushnumber(L, (lua_Number)time(NULL));
    return 1;
}

/*
 * H.system.now_iso() - current time as an ISO 8601 UTC string.
 *
 * Same shape as format_iso_time in utils_time.c, kept local so the
 * host API does not depend on a static helper there.
 */
static int H_lua_system_now_iso(lua_State* L) {
    time_t now = time(NULL);
    struct tm tm_buf;
    gmtime_r(&now, &tm_buf);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
    lua_pushstring(L, buf);
    return 1;
}

/*
 * H.system.instance_id() - stable per-process UUID.
 *
 * Generated on first call (16 random bytes from /dev/urandom, with
 * a time/pid hash fallback if urandom is unavailable), formatted as
 * a standard 36-char UUID, and cached for the process lifetime.
 *
 * The cache uses a static buffer; a simple "first call" flag (gated
 * by memset) avoids a pthread_once dependency in the host path.
 */
static int H_lua_system_instance_id(lua_State* L) {
    static char cached[UUID_STR_LEN];
    static bool initialized = false;

    if (!initialized) {
        unsigned char bytes[16] = {0};
        bool got_random = false;

        int fd = open("/dev/urandom", O_RDONLY);
        if (fd >= 0) {
            ssize_t n = read(fd, bytes, sizeof(bytes));
            close(fd);
            if (n == (ssize_t)sizeof(bytes)) {
                got_random = true;
            }
        }

        if (!got_random) {
            // Fallback: mix time, pid, and a small lcg to get 16 bytes
            // of pseudo-randomness. Not cryptographic but enough to
            // distinguish two server instances.
            unsigned long seed = (unsigned long)time(NULL) ^
                                 ((unsigned long)getpid() << 16);
            for (int i = 0; i < 16; i++) {
                seed = seed * 1103515245UL + 12345UL;
                bytes[i] = (unsigned char)((seed >> 16) & 0xff);
            }
        }

        // UUID v4 shape: set version (4) and variant (10xx) bits.
        bytes[6] = (unsigned char)((bytes[6] & 0x0f) | 0x40);
        bytes[8] = (unsigned char)((bytes[8] & 0x3f) | 0x80);

        snprintf(cached, sizeof(cached),
                 "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                 bytes[0], bytes[1], bytes[2], bytes[3],
                 bytes[4], bytes[5], bytes[6], bytes[7],
                 bytes[8], bytes[9], bytes[10], bytes[11],
                 bytes[12], bytes[13], bytes[14], bytes[15]);
        initialized = true;
    }

    lua_pushstring(L, cached);
    return 1;
}

/*
 * H.system.version() - the VERSION string baked into the build.
 */
static int H_lua_system_version(lua_State* L) {
    lua_pushstring(L, VERSION);
    return 1;
}

// H.gc.* implementations ///////////////////////////////////////////////////

/*
 * H.gc.collect() - run a full GC cycle and return the freed memory
 * in KB. Maps to lua_gc(L, LUA_GCCOLLECT, 0).
 *
 * Potentially expensive: the calling Lua state blocks until the
 * cycle completes. The progress hook deliberately does NOT call
 * this; long-running scripts (the Orchestrator) call it on their
 * own cadence in their scheduling loop.
 */
static int H_lua_gc_collect(lua_State* L) {
    lua_Integer kb = lua_gc(L, LUA_GCCOLLECT, 0);
    lua_pushnumber(L, (lua_Number)kb);
    return 1;
}

/*
 * H.gc.step() - perform a single GC step (lua_gc LUA_GCSTEP). Much
 * cheaper than collect(); the hook uses this to nudge the GC when
 * the soft memory limit is exceeded.
 */
static int H_lua_gc_step(lua_State* L) {
    lua_Integer kb = lua_gc(L, LUA_GCSTEP, 0);
    lua_pushnumber(L, (lua_Number)kb);
    return 1;
}

/*
 * H.gc.count() - return current memory in KB (lua_gc LUA_GCCOUNT).
 * Same primitive the hook uses, exposed to Lua for explicit
 * measurement (e.g. a script that wants to log memory before
 * allocating a large table).
 */
static int H_lua_gc_count(lua_State* L) {
    lua_Integer kb = lua_gc(L, LUA_GCCOUNT, 0);
    lua_pushnumber(L, (lua_Number)kb);
    return 1;
}

/*
 * H.gc.isrunning() - return true if the GC is currently running
 * (lua_gc LUA_GCISRUNNING). Mostly useful for diagnostics.
 */
static int H_lua_gc_isrunning(lua_State* L) {
    int running = lua_gc(L, LUA_GCISRUNNING, 0);
    lua_pushboolean(L, running != 0);
    return 1;
}

// H.set_current_state implementation ///////////////////////////////////////

/*
 * H.set_current_state(state) - voluntary progress report.
 *
 * Lua-side signature: (state: string)
 * C-side call:        H_lua_set_current_state(L)
 *
 * Validates that one string argument is present, then looks up the
 * per-state job context to find the running job's scoreboard entry
 * and copies the string out of Lua memory (UAF discipline from
 * Phase 1) before writing to the scoreboard.
 *
 * No-op cases (do not raise, do not log a warning):
 *   - No job context on the state (e.g. the Orchestrator, or a
 *     test that built a bare lua_State). The Orchestrator's docstring
 *     in lua_api.md calls this out explicitly.
 *   - Empty string (treated as "clear the field"; the C side
 *     converts it to NULL).
 *
 * Failure cases (log at LOG_LEVEL_ERROR, do not raise):
 *   - No argument or non-string first argument.
 *   - Unknown job_id (race with scoreboard_destroy; the C side
 *     frees the strdup'd copy).
 */
static int H_lua_set_current_state(lua_State* L) {
    int nargs = lua_gettop(L);
    if (nargs < 1) {
        log_this(SR_SCRIPTING, "H.set_current_state: missing string argument",
                 LOG_LEVEL_ERROR, 0);
        return 0;
    }
    if (!lua_isstring(L, 1)) {
        log_this(SR_SCRIPTING, "H.set_current_state: argument must be a string",
                 LOG_LEVEL_ERROR, 0);
        return 0;
    }

    H_lua_job_context* ctx = H_lua_get_job_context(L);
    if (!ctx || ctx->job_id[0] == '\0' || !ctx->scoreboard) {
        // No job context: the Orchestrator or a bare test state. This
        // is a documented no-op (lua_api.md Progress Reporting).
        return 0;
    }

    // Copy the string out of Lua memory before the scoreboard write.
    // UAF discipline from Phase 1: never let the C side hold a
    // pointer into Lua-owned memory.
    const char* state = lua_tostring(L, 1);
    if (!state) {
        return 0;
    }

    if (!scoreboard_update_current_state(ctx->scoreboard, ctx->job_id, state)) {
        log_this(SR_SCRIPTING,
                 "H.set_current_state: scoreboard update failed for job %s",
                 LOG_LEVEL_ERROR, 1, ctx->job_id);
    }

    return 0;
}

// H.sleep / H.shutdown_requested implementations ///////////////////////////

/*
 * H.sleep(milliseconds) - cooperative sleep.
 *
 * Phase 11: blocks the calling Lua state for up to `milliseconds`
 * ms, polling the subsystem's shutdown flag every 100 ms. Returns
 * early if the flag is set, so a long H.sleep wakes up promptly
 * when landing begins. Returns nothing.
 *
 * Non-positive or missing argument is treated as "no sleep" and
 * returns immediately. A non-numeric argument logs an error and
 * returns (does not raise).
 */
static int H_lua_sleep(lua_State* L) {
    int nargs = lua_gettop(L);
    int ms = 0;
    if (nargs < 1) {
        // No-op: treat as zero sleep. The host path is a leaf.
        return 0;
    }
    if (lua_isnumber(L, 1)) {
        double v = lua_tonumber(L, 1);
        if (v <= 0.0) {
            return 0;
        }
        ms = (int)v;
    } else {
        log_this(SR_SCRIPTING, "H.sleep: argument must be a number",
                 LOG_LEVEL_ERROR, 0);
        return 0;
    }

    // Poll the shutdown flag every 100 ms. usleep is interrupt-safe
    // and short enough that even a 5-second H.sleep wakes up within
    // ~100 ms of a landing signal.
    #define H_SLEEP_POLL_USEC (100 * 1000)
    int slept_us = 0;
    int total_us = ms * 1000;
    while (slept_us < total_us) {
        if (scripting_system_shutdown) {
            return 0;
        }
        int slice = total_us - slept_us;
        if (slice > H_SLEEP_POLL_USEC) {
            slice = H_SLEEP_POLL_USEC;
        }
        usleep((useconds_t)slice);
        slept_us += slice;
    }
    return 0;
    #undef H_SLEEP_POLL_USEC
}

/*
 * H.shutdown_requested() -> boolean.
 *
 * Phase 11: returns true if the scripting subsystem's shutdown flag
 * has been set. The Orchestrator's scheduling loop is expected to
 * check this on every iteration and exit cleanly. Cheap read of a
 * volatile sig_atomic_t; safe to call from any Lua state.
 */
static int H_lua_shutdown_requested(lua_State* L) {
    lua_pushboolean(L, scripting_system_shutdown != 0);
    return 1;
}

// H.scoreboard.* implementations //////////////////////////////////////////

/*
 * Resolve the scoreboard for the current Lua context.
 *
 * Workers have a per-state job context with a non-NULL scoreboard
 * pointer (set by the worker pool at hook install time). The
 * Orchestrator (and bare test states) have no job context; for
 * those, fall back to the subsystem's scripting_scoreboard global
 * so the Orchestrator can list/submit/cancel from its scheduling
 * loop. Returns NULL only if no scoreboard is available (subsystem
 * not running, or a test that did not initialize state).
 */
static Scoreboard* resolve_active_scoreboard(lua_State* L) {
    H_lua_job_context* ctx = H_lua_get_job_context(L);
    if (ctx && ctx->scoreboard) {
        return ctx->scoreboard;
    }
    return scripting_scoreboard;
}

/*
 * Push a ScoreboardEntry as a Lua table with all read-only fields.
 * Used by H.scoreboard.list and H.scoreboard.get. NULL entry is
 * a no-op.
 */
static void push_scoreboard_entry_as_table(lua_State* L, const ScoreboardEntry* e) {
    if (!e) {
        lua_pushnil(L);
        return;
    }
    lua_newtable(L);

    lua_pushstring(L, e->job_id);                  lua_setfield(L, -2, "job_id");
    lua_pushstring(L, e->script_name ? e->script_name : ""); lua_setfield(L, -2, "script_name");
    lua_pushstring(L, scoreboard_status_name(e->status));
                                                    lua_setfield(L, -2, "status");
    if (e->params_json) {
        lua_pushstring(L, e->params_json);
    } else {
        lua_pushnil(L);
    }
                                                    lua_setfield(L, -2, "params_json");
    lua_pushnumber(L, (lua_Number)e->created_at.tv_sec);
                                                    lua_setfield(L, -2, "created_at");
    if (e->started_at.tv_sec != 0) {
        lua_pushnumber(L, (lua_Number)e->started_at.tv_sec);
    } else {
        lua_pushnil(L);
    }
                                                    lua_setfield(L, -2, "started_at");
    if (e->finished_at.tv_sec != 0) {
        lua_pushnumber(L, (lua_Number)e->finished_at.tv_sec);
    } else {
        lua_pushnil(L);
    }
                                                    lua_setfield(L, -2, "finished_at");
    lua_pushinteger(L, (lua_Integer)e->instruction_count);
                                                    lua_setfield(L, -2, "instruction_count");
    lua_pushinteger(L, (lua_Integer)e->memory_used_kb);
                                                    lua_setfield(L, -2, "memory_used_kb");
    if (e->current_state) {
        lua_pushstring(L, e->current_state);
    } else {
        lua_pushnil(L);
    }
                                                    lua_setfield(L, -2, "current_state");
    lua_pushinteger(L, (lua_Integer)e->max_runtime_seconds);
                                                    lua_setfield(L, -2, "max_runtime_seconds");
    lua_pushboolean(L, e->kill_requested);
                                                    lua_setfield(L, -2, "kill_requested");
}

/*
 * H.scoreboard.list() -> array of jobs.
 */
static int H_lua_scoreboard_list(lua_State* L) {
    Scoreboard* sb = resolve_active_scoreboard(L);
    ScoreboardEntry** list = NULL;
    size_t count = 0;
    if (!scoreboard_list(sb, &list, &count)) {
        log_this(SR_SCRIPTING, "H.scoreboard.list: snapshot failed",
                 LOG_LEVEL_ERROR, 0);
        lua_newtable(L);
        return 1;
    }
    lua_newtable(L);
    for (size_t i = 0; i < count; i++) {
        push_scoreboard_entry_as_table(L, list[i]);
        lua_rawseti(L, -2, (lua_Integer)(i + 1));
    }
    scoreboard_list_free(list, count);
    return 1;
}

/*
 * H.scoreboard.get(job_id) -> job | nil
 */
static int H_lua_scoreboard_get(lua_State* L) {
    int nargs = lua_gettop(L);
    if (nargs < 1 || !lua_isstring(L, 1)) {
        log_this(SR_SCRIPTING, "H.scoreboard.get: missing or non-string job_id",
                 LOG_LEVEL_ERROR, 0);
        lua_pushnil(L);
        return 1;
    }
    const char* job_id = lua_tostring(L, 1);
    if (!job_id) {
        lua_pushnil(L);
        return 1;
    }
    Scoreboard* sb = resolve_active_scoreboard(L);
    ScoreboardEntry* e = scoreboard_find(sb, job_id);
    if (!e) {
        lua_pushnil(L);
        return 1;
    }
    push_scoreboard_entry_as_table(L, e);
    scoreboard_entry_free(e);
    return 1;
}

/*
 * H.scoreboard.submit(entry) -> job_id | nil
 *
 * entry is a table with at least script_name (string). params_json
 * (string) is optional. Uses scripting_submit_job_with_source
 * (Phase 7) under the hood; the source is empty unless the entry
 * carries a `source` field, in which case it is registered inline.
 *
 * The Orchestrator typically only knows the script name (the
 * source lives in the registry / DB loader) so the source field
 * is optional; for an Orchestrator that wants to submit ad-hoc
 * snippets, the source field provides an inline path.
 */
static int H_lua_scoreboard_submit(lua_State* L) {
    int nargs = lua_gettop(L);
    if (nargs < 1 || !lua_istable(L, 1)) {
        log_this(SR_SCRIPTING,
                 "H.scoreboard.submit: entry must be a table",
                 LOG_LEVEL_ERROR, 0);
        lua_pushnil(L);
        return 1;
    }
    // script_name
    lua_getfield(L, 1, "script_name");
    if (!lua_isstring(L, -1)) {
        log_this(SR_SCRIPTING,
                 "H.scoreboard.submit: entry.script_name must be a string",
                 LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        lua_pushnil(L);
        return 1;
    }
    const char* script_name = lua_tostring(L, -1);
    char script_name_buf[256];
    if (script_name) {
        snprintf(script_name_buf, sizeof(script_name_buf), "%s", script_name);
    } else {
        script_name_buf[0] = '\0';
    }
    lua_pop(L, 1);

    // params_json (optional)
    char* params_json = NULL;
    lua_getfield(L, 1, "params_json");
    if (lua_isstring(L, -1)) {
        const char* p = lua_tostring(L, -1);
        if (p) {
            params_json = strdup(p);
        }
    }
    lua_pop(L, 1);

    // source (optional, for inline-registration)
    char* source = NULL;
    lua_getfield(L, 1, "source");
    if (lua_isstring(L, -1)) {
        const char* s = lua_tostring(L, -1);
        if (s) {
            source = strdup(s);
        }
    }
    lua_pop(L, 1);

    // Copy strings out of Lua memory (UAF discipline from Phase 1):
    // the worker pool may submit and return before the caller
    // releases its arguments, and we are about to drop the Lua
    // values. Use the local copies instead.
    char* job_id;
    if (source) {
        job_id = scripting_submit_job_with_source(script_name_buf, source, params_json);
        free(source);
    } else {
        job_id = scripting_submit_job(script_name_buf, params_json);
    }
    free(params_json);

    if (!job_id) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushstring(L, job_id);
    free(job_id);
    return 1;
}

/*
 * H.scoreboard.cancel(job_id) -> boolean
 */
static int H_lua_scoreboard_cancel(lua_State* L) {
    int nargs = lua_gettop(L);
    if (nargs < 1 || !lua_isstring(L, 1)) {
        log_this(SR_SCRIPTING,
                 "H.scoreboard.cancel: missing or non-string job_id",
                 LOG_LEVEL_ERROR, 0);
        lua_pushboolean(L, 0);
        return 1;
    }
    const char* job_id = lua_tostring(L, 1);
    if (!job_id) {
        lua_pushboolean(L, 0);
        return 1;
    }
    Scoreboard* sb = resolve_active_scoreboard(L);
    bool ok = scoreboard_request_kill(sb, job_id);
    lua_pushboolean(L, ok ? 1 : 0);
    return 1;
}

// Install functions ////////////////////////////////////////////////////////

/*
 * Populate H.log with the six level functions.
 *
 * Assumes H.log already exists as a table; if it does not, the
 * operation is logged and skipped (caller should ensure H and the
 * placeholder sub-tables are created first).
 */
void H_lua_install_log(lua_State* L) {
    if (!L) return;

    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_log: H table missing", LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }
    lua_getfield(L, -1, "log");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_log: H.log not a table", LOG_LEVEL_ERROR, 0);
        lua_pop(L, 2);
        return;
    }

    lua_pushcfunction(L, H_lua_log_trace); lua_setfield(L, -2, "trace");
    lua_pushcfunction(L, H_lua_log_debug); lua_setfield(L, -2, "debug");
    lua_pushcfunction(L, H_lua_log_info);  lua_setfield(L, -2, "info");
    lua_pushcfunction(L, H_lua_log_warn);  lua_setfield(L, -2, "warn");
    lua_pushcfunction(L, H_lua_log_error); lua_setfield(L, -2, "error");
    lua_pushcfunction(L, H_lua_log_fatal); lua_setfield(L, -2, "fatal");

    lua_pop(L, 2); // pop H.log and H
}

/*
 * Populate H.system with the five utility functions.
 */
void H_lua_install_system(lua_State* L) {
    if (!L) return;

    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_system: H table missing", LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }
    lua_getfield(L, -1, "system");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_system: H.system not a table", LOG_LEVEL_ERROR, 0);
        lua_pop(L, 2);
        return;
    }

    lua_pushcfunction(L, H_lua_system_uptime);      lua_setfield(L, -2, "uptime");
    lua_pushcfunction(L, H_lua_system_now);         lua_setfield(L, -2, "now");
    lua_pushcfunction(L, H_lua_system_now_iso);     lua_setfield(L, -2, "now_iso");
    lua_pushcfunction(L, H_lua_system_instance_id); lua_setfield(L, -2, "instance_id");
    lua_pushcfunction(L, H_lua_system_version);     lua_setfield(L, -2, "version");

    lua_pop(L, 2); // pop H.system and H
}

/*
 * Populate H.gc with the four explicit-GC-control functions.
 */
void H_lua_install_gc(lua_State* L) {
    if (!L) return;

    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_gc: H table missing", LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }
    lua_getfield(L, -1, "gc");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_gc: H.gc not a table", LOG_LEVEL_ERROR, 0);
        lua_pop(L, 2);
        return;
    }

    lua_pushcfunction(L, H_lua_gc_collect);   lua_setfield(L, -2, "collect");
    lua_pushcfunction(L, H_lua_gc_step);      lua_setfield(L, -2, "step");
    lua_pushcfunction(L, H_lua_gc_count);     lua_setfield(L, -2, "count");
    lua_pushcfunction(L, H_lua_gc_isrunning); lua_setfield(L, -2, "isrunning");

    lua_pop(L, 2); // pop H.gc and H
}

/*
 * Populate H.set_current_state with the voluntary-progress-report
 * function. Replaces the Phase 3 placeholder sub-table of the same
 * name (Lua allows the field to be reassigned from a table to a
 * function; the sub-table becomes garbage).
 */
void H_lua_install_set_current_state(lua_State* L) {
    if (!L) return;

    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_set_current_state: H table missing",
                 LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }

    // No sub-table to descend into: H.set_current_state is a
    // top-level function on H. lua_setfield replaces the Phase 3
    // placeholder sub-table with the cfunction.
    lua_pushcfunction(L, H_lua_set_current_state);
    lua_setfield(L, -2, "set_current_state");

    lua_pop(L, 1); // pop H
}

/*
 * Populate H.sleep and H.shutdown_requested. Both are top-level
 * functions on H, replacing the Phase 3 placeholder sub-tables of
 * the same names.
 */
void H_lua_install_sleep_shutdown(lua_State* L) {
    if (!L) return;

    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_sleep_shutdown: H table missing",
                 LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }

    lua_pushcfunction(L, H_lua_sleep);
    lua_setfield(L, -2, "sleep");

    lua_pushcfunction(L, H_lua_shutdown_requested);
    lua_setfield(L, -2, "shutdown_requested");

    lua_pop(L, 1); // pop H
}

/*
 * Populate H.scoreboard with the four scoreboard access functions.
 * The sub-table itself was created in Phase 3 as a placeholder; this
 * replaces its empty contents with the real functions.
 */
void H_lua_install_scoreboard(lua_State* L) {
    if (!L) return;

    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_scoreboard: H table missing",
                 LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }
    lua_getfield(L, -1, "scoreboard");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_scoreboard: H.scoreboard not a table",
                 LOG_LEVEL_ERROR, 0);
        lua_pop(L, 2);
        return;
    }

    lua_pushcfunction(L, H_lua_scoreboard_list);   lua_setfield(L, -2, "list");
    lua_pushcfunction(L, H_lua_scoreboard_get);    lua_setfield(L, -2, "get");
    lua_pushcfunction(L, H_lua_scoreboard_submit); lua_setfield(L, -2, "submit");
    lua_pushcfunction(L, H_lua_scoreboard_cancel); lua_setfield(L, -2, "cancel");

    lua_pop(L, 2); // pop H.scoreboard and H
}

// ----------------------------------------------------------------------------
// H.package.searcher implementation (Phase 11g)
// ----------------------------------------------------------------------------

/*
 * Split a module name on the first '.' into group and script.
 * Returns true on success and writes heap-allocated copies into
 * *group_out and *script_out (caller frees). Returns false on
 * malformed names (no dot, empty group, empty script).
 */
static bool split_module_name(const char* name,
                              char** group_out,
                              char** script_out) {
    if (!name || !group_out || !script_out) {
        return false;
    }
    const char* dot = strchr(name, '.');
    if (!dot || dot == name || dot[1] == '\0') {
        return false;
    }
    size_t group_len = (size_t)(dot - name);
    char* group = malloc(group_len + 1);
    if (!group) {
        return false;
    }
    memcpy(group, name, group_len);
    group[group_len] = '\0';

    const char* script = dot + 1;
    char* script_copy = strdup(script);
    if (!script_copy) {
        free(group);
        return false;
    }

    *group_out = group;
    *script_out = script_copy;
    return true;
}

/*
 * package.searchers entry for DB-backed `require("group.script")`.
 *
 * Lua calls this with the module name. On success it returns the
 * compiled chunk function (which Lua's require then calls to run the
 * module). On failure it returns a single error string, so Lua
 * appends the message to its "module not found" diagnostics and
 * tries the next searcher.
 *
 * The source is cached process-wide in scripting_source_cache, so
 * repeated requires of the same module do not hit the DB.
 */
static int H_lua_package_searcher(lua_State* L) {
    const char* module_name = luaL_checkstring(L, 1);

    char* group = NULL;
    char* script = NULL;
    if (!split_module_name(module_name, &group, &script)) {
        lua_pushfstring(L,
                        "no module '%s' in scripts table: "
                        "invalid name (expected group.script)",
                        module_name);
        return 1;
    }

    if (!scripting_source_cache) {
        lua_pushfstring(L,
                        "no module '%s' in scripts table: "
                        "source cache not initialized",
                        module_name);
        free(group);
        free(script);
        return 1;
    }

    const char* source = source_cache_get(scripting_source_cache, group, script);
    char* fetched_source = NULL;
    if (!source) {
        if (!app_config || !app_config->scripting.DefaultDatabase ||
            app_config->scripting.DefaultDatabase[0] == '\0') {
            lua_pushfstring(L,
                            "no module '%s' in scripts table: "
                            "DefaultDatabase not configured",
                            module_name);
            free(group);
            free(script);
            return 1;
        }
        fetched_source = scripting_fetch_script_source(
            group, script, app_config->scripting.DefaultDatabase, 5);
        if (!fetched_source) {
            lua_pushfstring(L,
                            "no module '%s' in scripts table: "
                            "row not found or DB unavailable",
                            module_name);
            free(group);
            free(script);
            return 1;
        }
        source_cache_put(scripting_source_cache, group, script, fetched_source);
        source = fetched_source;
    }

    size_t source_len = strlen(source);
    int rc = luaL_loadbufferx(L, source, source_len, module_name, NULL);

    // The temporary copies from the split are no longer needed.
    free(group);
    free(script);
    if (fetched_source) {
        free(fetched_source);
    }

    if (rc != LUA_OK) {
        // luaL_loadbufferx left an error string on the stack.
        // Return it as the searcher's error message so Lua appends it.
        return 1;
    }

    // Return the compiled chunk function. Lua's require will call it.
    return 1;
}

/*
 * Install the DB-backed package searcher at the end of
 * package.searchers. Gated by app_config->scripting.AllowDBModuleLoad;
 * if false, the function is a no-op and `require` keeps its default
 * behavior (preload + file-based searchers only).
 */
void H_lua_install_package(lua_State* L) {
    if (!L) {
        return;
    }
    if (!app_config || !app_config->scripting.AllowDBModuleLoad) {
        return;
    }

    lua_getglobal(L, "package");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_package: package table missing",
                 LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }
    lua_getfield(L, -1, "searchers");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_package: package.searchers not a table",
                 LOG_LEVEL_ERROR, 0);
        lua_pop(L, 2);
        return;
    }

    lua_pushcfunction(L, H_lua_package_searcher);
    lua_seti(L, -2, (lua_Integer)lua_rawlen(L, -2) + 1);

    lua_pop(L, 2); // pop package.searchers and package
}

// ----------------------------------------------------------------------------
// Phase 13: H.query / H.altquery / H.authquery / H.wait
// ----------------------------------------------------------------------------

/*
 * Convert a Lua table at `arg` to Hydrogen's parameter_json format.
 *
 * Lua value -> typed JSON mapping:
 *   integer number -> INTEGER
 *   non-integer number -> FLOAT
 *   string -> STRING
 *   boolean -> BOOLEAN
 *   nil fields are omitted
 *   nested tables / functions / userdata are skipped with a log
 *
 * Returns a heap-allocated JSON string (caller frees) or NULL on
 * error. The JSON object has keys INTEGER, STRING, BOOLEAN, FLOAT;
 * only the type groups that have at least one entry are emitted.
 *
 * The "skip and log" behavior for unsupported types is deliberate:
 * the host log path is a leaf (Phase 6 rule). A bad param value
 * becomes a partial JSON object (with the bad fields omitted) rather
 * than a hard failure, so a script with one bad param out of ten
 * still gets its nine valid params through to the engine.
 */
static char* H_lua_params_to_json(lua_State* L, int arg) {
    if (!lua_istable(L, arg)) {
        return NULL;
    }

    json_t* int_obj = json_object();
    json_t* str_obj = json_object();
    json_t* bool_obj = json_object();
    json_t* float_obj = json_object();
    if (!int_obj || !str_obj || !bool_obj || !float_obj) {
        if (int_obj) json_decref(int_obj);
        if (str_obj) json_decref(str_obj);
        if (bool_obj) json_decref(bool_obj);
        if (float_obj) json_decref(float_obj);
        return NULL;
    }

    int idx = 0;
    lua_pushnil(L);
    while (lua_next(L, arg) != 0) {
        // key at -2, value at -1
        const char* key = lua_tostring(L, -2);
        if (!key) {
            lua_pop(L, 1);
            continue;
        }
        int t = lua_type(L, -1);
        if (t == LUA_TNUMBER) {
            double d = lua_tonumber(L, -1);
            // Use lua_tointegerx for safe integer detection
            int is_int = 0;
            lua_Integer li = lua_tointegerx(L, -1, &is_int);
            if (is_int) {
                json_object_set_new(int_obj, key, json_integer(li));
            } else {
                json_object_set_new(float_obj, key, json_real(d));
            }
        } else if (t == LUA_TSTRING) {
            const char* s = lua_tostring(L, -1);
            json_object_set_new(str_obj, key, json_string(s ? s : ""));
        } else if (t == LUA_TBOOLEAN) {
            json_object_set_new(bool_obj, key, json_boolean(lua_toboolean(L, -1)));
        } else if (t == LUA_TNIL) {
            // omit
        } else {
            log_this(SR_LUA,
                     "H.*query: skipping unsupported param type for key '%s'",
                     LOG_LEVEL_ALERT, 1, key);
        }
        lua_pop(L, 1); // pop value, keep key for next iteration
        idx++;
        if (idx > 1000) {
            log_this(SR_LUA, "H.*query: params table too large (>1000 entries)",
                     LOG_LEVEL_ALERT, 0);
            break;
        }
    }

    json_t* root = json_object();
    if (json_object_size(int_obj) > 0) {
        json_object_set_new(root, "INTEGER", int_obj);
    } else {
        json_decref(int_obj);
    }
    if (json_object_size(str_obj) > 0) {
        json_object_set_new(root, "STRING", str_obj);
    } else {
        json_decref(str_obj);
    }
    if (json_object_size(bool_obj) > 0) {
        json_object_set_new(root, "BOOLEAN", bool_obj);
    } else {
        json_decref(bool_obj);
    }
    if (json_object_size(float_obj) > 0) {
        json_object_set_new(root, "FLOAT", float_obj);
    } else {
        json_decref(float_obj);
    }

    char* out = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    return out;
}

/*
 * Convert a data_json string (JSON array of row objects) to a Lua
 * table. The pushed table has:
 *   rows = { row1, row2, ... }  where each row is a Lua table with
 *          column name keys and the column values.
 *
 * NULL values become Lua nil, numbers become Lua numbers, strings
 * become Lua strings, booleans become Lua booleans. Objects (nested
 * tables) are pushed as Lua tables recursively (shallow — one level
 * is sufficient for flat result rows; nested objects are kept as
 * Lua tables without further recursion into arrays).
 *
 * On JSON parse failure, pushes an empty rows table and logs the
 * error. The caller (H.wait) decides whether to treat the parse
 * failure as a handle error.
 */
static void push_json_value_as_lua(lua_State* L, json_t* val);

static void push_json_object_as_table(lua_State* L, json_t* obj) {
    if (!json_is_object(obj)) {
        lua_pushnil(L);
        return;
    }
    lua_newtable(L);
    const char* key = NULL;
    json_t* val = NULL;
    json_object_foreach((json_t*)obj, key, val) {
        push_json_value_as_lua(L, val);
        lua_setfield(L, -2, key);
    }
}

static void push_json_array_as_table(lua_State* L, json_t* arr) {
    lua_newtable(L);
    size_t idx = 0;
    json_t* val = NULL;
    size_t i = 1;
    json_array_foreach((json_t*)arr, i, val) {
        push_json_value_as_lua(L, val);
        lua_rawseti(L, -2, (lua_Integer)idx + 1);
        idx++;
    }
}

static void push_json_value_as_lua(lua_State* L, json_t* val) {
    if (!val) {
        lua_pushnil(L);
        return;
    }
    switch (json_typeof(val)) {
    case JSON_NULL:
        lua_pushnil(L);
        break;
    case JSON_TRUE:
        lua_pushboolean(L, 1);
        break;
    case JSON_FALSE:
        lua_pushboolean(L, 0);
        break;
    case JSON_INTEGER:
        lua_pushinteger(L, (lua_Integer)json_integer_value(val));
        break;
    case JSON_REAL:
        lua_pushnumber(L, (lua_Number)json_real_value(val));
        break;
    case JSON_STRING:
        lua_pushstring(L, json_string_value(val));
        break;
    case JSON_ARRAY:
        push_json_array_as_table(L, val);
        break;
    case JSON_OBJECT:
        push_json_object_as_table(L, val);
        break;
    default:
        lua_pushnil(L);
        break;
    }
}

/*
 * Parse data_json (a JSON array string) and push a result table onto
 * the Lua stack. The result table has:
 *   rows           = { row1, row2, ... }
 *   affected_rows  = N   (from the engine; 0 for SELECT or empty)
 *
 * On parse failure, pushes a table with rows = {} and logs the
 * error. Returns 1 (number of values pushed).
 *
 * Exposed (non-static) so it can be tested directly via Unity. The
 * project rule "Don't use static functions if possible as we can't
 * test those" applies.
 */
int H_lua_build_result_table(lua_State* L, const char* data_json, int affected_rows) {
    lua_newtable(L);
    lua_pushstring(L, "rows");
    if (data_json && *data_json) {
        json_error_t err;
        json_t* arr = json_loads(data_json, 0, &err);
        if (arr && json_is_array(arr)) {
            size_t n = json_array_size(arr);
            lua_newtable(L); // rows
            for (size_t i = 0; i < n; i++) {
                push_json_value_as_lua(L, json_array_get(arr, i));
                lua_rawseti(L, -2, (lua_Integer)(i + 1));
            }
            json_decref(arr);
        } else {
            log_this(SR_LUA, "H.*query: data_json parse failed: %s",
                     LOG_LEVEL_ALERT, 1,
                     arr ? "not a JSON array" : err.text);
            if (arr) json_decref(arr);
            lua_newtable(L); // empty rows
        }
    } else {
        lua_newtable(L); // empty rows
    }
    lua_settable(L, -3);

    // Phase 14: surface affected_rows from the engine. For write
    // statements (UPDATE/INSERT/DELETE) this is the engine's reported
    // count; for SELECTs it is 0. Atomic task-claim recipes rely on
    // exactly-one-winner-sees-1 semantics, e.g.:
    //   local res = H.wait(H.query(
    //     [[ UPDATE tasks SET status='taken'
    //            WHERE id=:id AND status='open' ]],
    //     { id = task_id }))
    //   if res.affected_rows == 1 then -- we own it
    lua_pushstring(L, "affected_rows");
    lua_pushinteger(L, (lua_Integer)affected_rows);
    lua_settable(L, -3);

    return 1;
}

/*
 * Resolve the DatabaseQueue for a query. Returns NULL on failure and
 * pushes an error string (the handle will pick it up as its error).
 *
 * For db_name == NULL: uses config->scripting.DefaultDatabase.
 *   - If DefaultDatabase is set, uses it.
 *   - If DefaultDatabase is empty and exactly one DB is configured,
 *     uses that DB (single-DB fallback, matching orchestrator.c).
 *   - Otherwise returns NULL with "no default database" error.
 */
static DatabaseQueue* resolve_db_queue(const char* db_name,
                                        char** err_out) {
    if (err_out) *err_out = NULL;

    const char* target = db_name;
    if (!target || !*target) {
        if (!app_config) {
            if (err_out) {
                *err_out = strdup("H.query: no app_config available");
            }
            return NULL;
        }
        target = app_config->scripting.DefaultDatabase;
        if (!target || !*target) {
            // Single-DB fallback: with one database the choice is
            // unambiguous (same pattern as orchestrator.c).
            if (app_config->databases.connection_count == 1 &&
                app_config->databases.connections[0].name &&
                app_config->databases.connections[0].name[0] != '\0') {
                target = app_config->databases.connections[0].name;
            } else {
                if (err_out) {
                    *err_out = strdup("H.query: no database specified and no "
                                      "scripting.DefaultDatabase configured");
                }
                return NULL;
            }
        }
    }

    if (!global_queue_manager) {
        if (err_out) {
            *err_out = strdup("H.query: database queue manager not available");
        }
        return NULL;
    }

    DatabaseQueue* dbq = database_queue_manager_get_database(
        global_queue_manager, target);
    if (!dbq) {
        if (err_out) {
            char buf[256];
            snprintf(buf, sizeof(buf),
                     "H.query: database '%s' not found", target);
            *err_out = strdup(buf);
        }
        return NULL;
    }
    return dbq;
}

/*
 * Submit a raw SQL query to the database queue. Creates a handle,
 * submits, and pushes the handle userdata onto the Lua stack.
 *
 * On any failure (resolution, allocation, submit), pushes a handle
 * whose `error` field is set, so H.wait will report the error
 * without touching the queue. Returns 1 on success (one value
 * pushed: the handle userdata) and 0 if a handle could not be
 * pushed (caller must handle the empty stack).
 */
static int H_lua_submit_query(lua_State* L,
                               const char* db_name,
                               const char* sql,
                               const char* params_json,
                               int timeout_seconds __attribute__((unused)),
                               const char* call_label) {
    if (!L || !sql) {
        H_Handle* h = H_Handle_new(L, H_HK_QUERY);
        if (h) {
            h->error = strdup("H.query: NULL sql argument");
        }
        return 1;
    }

    char* resolve_err = NULL;
    DatabaseQueue* dbq = resolve_db_queue(db_name, &resolve_err);
    if (!dbq) {
        H_Handle* h = H_Handle_new(L, H_HK_QUERY);
        if (h && resolve_err) {
            h->error = resolve_err;
        } else {
            free(resolve_err);
        }
        return 1;
    }

    char* query_id = generate_query_id();
    if (!query_id) {
        H_Handle* h = H_Handle_new(L, H_HK_QUERY);
        if (h) {
            h->error = strdup("H.query: failed to generate query_id");
        }
        return 1;
    }

    DatabaseQuery db_query = {
        .query_id = query_id,
        .query_template = strdup(sql),
        .parameter_json = params_json ? strdup(params_json) : NULL,
        .queue_type_hint = 0,   // route to the default child queue
        .submitted_at = time(NULL),
        .processed_at = 0,
        .retry_count = 0,
        .error_message = NULL
    };
    if (!db_query.query_template) {
        free(query_id);
        free(db_query.parameter_json);
        H_Handle* h = H_Handle_new(L, H_HK_QUERY);
        if (h) {
            h->error = strdup("H.query: failed to duplicate SQL");
        }
        return 1;
    }

    if (!database_queue_submit_query(dbq, &db_query)) {
        free(query_id);
        free(db_query.query_template);
        free(db_query.parameter_json);
        H_Handle* h = H_Handle_new(L, H_HK_QUERY);
        if (h) {
            char buf[128];
            snprintf(buf, sizeof(buf), "%s: database submit failed", call_label);
            h->error = strdup(buf);
        }
        return 1;
    }

    // Submit succeeded; build the success handle.
    H_Handle* h = H_Handle_new(L, H_HK_QUERY);
    if (!h) {
        // Handle allocation failed after submit. The query is in
        // flight; let the DQM process it. We leak the query_id
        // intentionally to avoid a double-free race with the DQM.
        return 0; // pushed nothing; caller must handle missing handle
    }
    h->query_id = query_id;        // takes ownership
    h->db_queue = dbq;            // not owned
    // query_template and parameter_json were consumed by the queue
    // via serialization; do not free them here.
    return 1;
}

/*
 * Get the default query timeout from config. Falls back to 30s when
 * config is unavailable or the value is non-positive.
 */
static int get_default_query_timeout(void) {
    if (app_config && app_config->scripting.DefaultQueryTimeout > 0) {
        return app_config->scripting.DefaultQueryTimeout;
    }
    return 30;
}

/*
 * H.query(sql, params, opts?) -> handle
 *
 * Submits `sql` to config->scripting.DefaultDatabase. `params` is
 * an optional Lua table of named parameters. `opts.timeout` (seconds)
 * defaults to config->scripting.DefaultQueryTimeout (30s fallback).
 */
static int H_lua_query(lua_State* L) {
    int nargs = lua_gettop(L);
    if (nargs < 1) {
        H_Handle* h = H_Handle_new(L, H_HK_QUERY);
        if (h) h->error = strdup("H.query: missing sql argument");
        return 1;
    }
    const char* sql = luaL_checkstring(L, 1);
    if (!sql) {
        H_Handle* h = H_Handle_new(L, H_HK_QUERY);
        if (h) h->error = strdup("H.query: sql is not a string");
        return 1;
    }

    char* params_json = NULL;
    if (nargs >= 2 && !lua_isnil(L, 2)) {
        params_json = H_lua_params_to_json(L, 2);
    }

    int timeout = get_default_query_timeout();
    if (nargs >= 3 && lua_istable(L, 3)) {
        lua_getfield(L, 3, "timeout");
        if (lua_isnumber(L, -1)) {
            int t = (int)lua_tonumber(L, -1);
            if (t > 0) timeout = t;
        }
        lua_pop(L, 1);
    }

    int pushed = H_lua_submit_query(L, NULL, sql, params_json, timeout, "H.query");
    free(params_json);
    if (pushed == 0) {
        // H_lua_submit_query could not allocate a handle after
        // submitting the query. We have nothing to push; the caller
        // (including the sync wrappers) detects this and returns an
        // error without touching the queue.
        return 0;
    }
    return pushed;
}

/*
 * H.altquery(database_name, sql, params, opts?) -> handle
 *
 * Like H.query but the database name is explicit. database_name
 * must be a non-empty string and must match a configured database.
 */
static int H_lua_altquery(lua_State* L) {
    int nargs = lua_gettop(L);
    if (nargs < 2) {
        H_Handle* h = H_Handle_new(L, H_HK_QUERY);
        if (h) {
            h->error = strdup("H.altquery: missing database_name or sql argument");
        }
        return 1;
    }
    const char* db_name = luaL_checkstring(L, 1);
    const char* sql = luaL_checkstring(L, 2);
    if (!db_name || !*db_name) {
        H_Handle* h = H_Handle_new(L, H_HK_QUERY);
        if (h) h->error = strdup("H.altquery: database_name is empty");
        return 1;
    }
    if (!sql) {
        H_Handle* h = H_Handle_new(L, H_HK_QUERY);
        if (h) h->error = strdup("H.altquery: sql is not a string");
        return 1;
    }

    char* params_json = NULL;
    if (nargs >= 3 && !lua_isnil(L, 3)) {
        params_json = H_lua_params_to_json(L, 3);
    }

    int timeout = get_default_query_timeout();
    if (nargs >= 4 && lua_istable(L, 4)) {
        lua_getfield(L, 4, "timeout");
        if (lua_isnumber(L, -1)) {
            int t = (int)lua_tonumber(L, -1);
            if (t > 0) timeout = t;
        }
        lua_pop(L, 1);
    }

    int pushed = H_lua_submit_query(L, db_name, sql, params_json, timeout, "H.altquery");
    free(params_json);
    if (pushed == 0) {
        return 0;
    }
    return pushed;
}

/*
 * Validate a JWT and return the database claim, or NULL on failure.
 *
 * This wraps the existing auth_service_jwt validator. The token
 * must be a valid, non-expired, non-revoked JWT signed with
 * Hydrogen's key. The `database` claim is extracted from the
 * payload and returned as a strdup'd string (caller frees).
 *
 * If validation fails, *err_out is set to a human-readable error.
 */
static char* validate_jwt_and_get_db(const char* token, char** err_out) {
    if (err_out) *err_out = NULL;
    if (!token || !*token) {
        if (err_out) *err_out = strdup("H.authquery: empty token");
        return NULL;
    }

    jwt_validation_result_t result = validate_jwt(token, NULL);
    if (!result.valid) {
        if (err_out) {
            char buf[256];
            snprintf(buf, sizeof(buf),
                     "H.authquery: JWT validation failed (error %d)",
                     (int)result.error);
            *err_out = strdup(buf);
        }
        if (result.claims) {
            // free claims if provided
        }
        return NULL;
    }
    if (!result.claims || !result.claims->database ||
        !*result.claims->database) {
        if (err_out) {
            *err_out = strdup("H.authquery: JWT has no database claim");
        }
        return NULL;
    }
    char* db = strdup(result.claims->database);
    return db;
}

/*
 * H.authquery(token, sql, params, opts?) -> handle
 *
 * Validates the JWT, extracts the database from its claims, and
 * submits the query to that database. The token is also checked
 * for revocation.
 */
static int H_lua_authquery(lua_State* L) {
    int nargs = lua_gettop(L);
    if (nargs < 2) {
        H_Handle* h = H_Handle_new(L, H_HK_QUERY);
        if (h) {
            h->error = strdup("H.authquery: missing token or sql argument");
        }
        return 1;
    }
    const char* token = luaL_checkstring(L, 1);
    const char* sql = luaL_checkstring(L, 2);
    if (!token) {
        H_Handle* h = H_Handle_new(L, H_HK_QUERY);
        if (h) h->error = strdup("H.authquery: token is not a string");
        return 1;
    }
    if (!sql) {
        H_Handle* h = H_Handle_new(L, H_HK_QUERY);
        if (h) h->error = strdup("H.authquery: sql is not a string");
        return 1;
    }

    char* jwt_err = NULL;
    char* db = validate_jwt_and_get_db(token, &jwt_err);
    if (!db) {
        H_Handle* h = H_Handle_new(L, H_HK_QUERY);
        if (h && jwt_err) {
            h->error = jwt_err;
        } else {
            free(jwt_err);
        }
        return 1;
    }

    char* params_json = NULL;
    if (nargs >= 3 && !lua_isnil(L, 3)) {
        params_json = H_lua_params_to_json(L, 3);
    }

    int timeout = get_default_query_timeout();
    if (nargs >= 4 && lua_istable(L, 4)) {
        lua_getfield(L, 4, "timeout");
        if (lua_isnumber(L, -1)) {
            int t = (int)lua_tonumber(L, -1);
            if (t > 0) timeout = t;
        }
        lua_pop(L, 1);
    }

    int pushed = H_lua_submit_query(L, db, sql, params_json, timeout, "H.authquery");
    free(db);
    free(params_json);
    if (pushed == 0) {
        return 0;
    }
    return pushed;
}

/*
 * Wait on a single handle and push the result/error pair. Returns 2
 * (one result + one error). Dispatches on `handle->kind`:
 *
 *   H_HK_QUERY -> database_queue_await_result + build result table
 *   H_HK_HTTP  -> blocking libcurl call + build result table (Phase 16)
 *
 * If the handle is in an error state or not yet consumed, pushes
 * nil + error. The same shape is produced for both kinds so the
 * caller (H.wait or a *_sync wrapper) can treat the two uniformly.
 */
// Forward declaration: H_lua_http_wait_one is defined further down
// in the file but H_lua_wait_one dispatches on kind.
static int H_lua_http_wait_one(lua_State* L, H_Handle* h);
static int H_lua_wait_one(lua_State* L, H_Handle* h) {
    if (!h) {
        lua_pushnil(L);
        lua_pushstring(L, "H.wait: invalid handle");
        return 2;
    }
    if (h->consumed) {
        lua_pushnil(L);
        lua_pushstring(L, "H.wait: handle already consumed");
        return 2;
    }
    if (h->error) {
        lua_pushnil(L);
        lua_pushstring(L, h->error);
        h->consumed = true;
        return 2;
    }
    if (h->kind == H_HK_HTTP) {
        return H_lua_http_wait_one(L, h);
    }
    if (!h->query_id || !h->db_queue) {
        lua_pushnil(L);
        lua_pushstring(L, "H.wait: handle has no pending query");
        h->consumed = true;
        return 2;
    }

    // Look up the timeout from app_config (default 30s).
    int timeout = get_default_query_timeout();

    DatabaseQuery* result_q = database_queue_await_result(
        h->db_queue, h->query_id, timeout);
    if (!result_q) {
        char buf[64];
        snprintf(buf, sizeof(buf), "H.wait: timeout after %ds", timeout);
        lua_pushnil(L);
        lua_pushstring(L, buf);
        h->consumed = true;
        return 2;
    }

    h->consumed = true;

    // Check for error_message.
    if (result_q->error_message) {
        lua_pushnil(L);
        lua_pushstring(L, result_q->error_message);
    } else {
        // result_q->query_template holds the data_json (see
        // database_queue_await_result docs). result_q->affected_rows
        // is propagated from QueryResult.affected_rows by the await
        // path (Phase 14).
        H_lua_build_result_table(L, result_q->query_template, result_q->affected_rows);
        lua_pushnil(L);
    }

    // Free the heap-allocated DatabaseQuery* from await_result.
    free(result_q->query_id);
    free(result_q->query_template);
    free(result_q->error_message);
    free(result_q);

    return 2;
}

/*
 * H.wait(handle1, handle2, ...) -> r1, r2, ..., rN, e1, e2, ..., eN
 *
 * Blocks until every handle is ready. For N handles, returns 2N
 * values: N results, then N errors. Each result is a table (or nil
 * on error); each error is a string (or nil on success).
 *
 * Handles are consumed after wait; a second wait on the same
 * handle returns nil + "already consumed".
 */
static int H_lua_wait(lua_State* L) {
    int n = lua_gettop(L);
    if (n == 0) {
        return 0;
    }

    H_Handle** handles = calloc((size_t)n, sizeof(H_Handle*));
    char** errors = calloc((size_t)n, sizeof(char*));
    if (!handles || !errors) {
        free(handles);
        free(errors);
        for (int i = 0; i < n; i++) {
            lua_pushnil(L);
        }
        for (int i = 0; i < n; i++) {
            lua_pushstring(L, "H.wait: allocation failure");
        }
        return 2 * n;
    }

    for (int i = 0; i < n; i++) {
        handles[i] = H_Handle_check(L, i + 1);
    }

    // Collect results and errors.
    for (int i = 0; i < n; i++) {
        H_Handle* h = handles[i];
        if (!h) {
            lua_pushnil(L);
            errors[i] = strdup("H.wait: argument is not a valid handle");
            continue;
        }
        if (h->consumed) {
            lua_pushnil(L);
            errors[i] = strdup("H.wait: handle already consumed");
            continue;
        }
        if (h->error) {
            lua_pushnil(L);
            errors[i] = strdup(h->error);
            h->consumed = true;
            continue;
        }
        if (h->kind == H_HK_HTTP) {
            // Phase 16: HTTP handles are resolved via
            // H_lua_http_wait_one, which pushes 2 values (result,
            // err) onto the stack. To keep the variadic loop's
            // "one result per handle, errors[] on the side"
            // invariant, we move the err string to errors[] and pop
            // it from the stack, leaving the result table where
            // the QUERY path's result table would be.
            // H_lua_http_wait_one always returns 2 (it always
            // pushes result + err). Move the err string to
            // errors[] and pop it from the stack, leaving the
            // result table where the QUERY path's result table
            // would be.
            (void)H_lua_wait_one(L, h);
            if (lua_isnil(L, -1)) {
                errors[i] = NULL;
            } else if (lua_isstring(L, -1)) {
                errors[i] = strdup(lua_tostring(L, -1));
            } else {
                errors[i] = NULL;
            }
            lua_pop(L, 1);
            h->consumed = true;
            continue;
        }
        if (!h->query_id || !h->db_queue) {
            lua_pushnil(L);
            errors[i] = strdup("H.wait: handle has no pending query");
            h->consumed = true;
            continue;
        }

        int timeout = get_default_query_timeout();
        DatabaseQuery* result_q = database_queue_await_result(
            h->db_queue, h->query_id, timeout);
        h->consumed = true;
        if (!result_q) {
            char buf[64];
            snprintf(buf, sizeof(buf), "H.wait: timeout after %ds", timeout);
            lua_pushnil(L);
            errors[i] = strdup(buf);
            continue;
        }
        if (result_q->error_message) {
            lua_pushnil(L);
            errors[i] = strdup(result_q->error_message);
        } else {
            H_lua_build_result_table(L, result_q->query_template, result_q->affected_rows);
            errors[i] = NULL; // success
        }
        free(result_q->query_id);
        free(result_q->query_template);
        free(result_q->error_message);
        free(result_q);
    }

    // Push the error strings (N values, one per handle).
    for (int i = 0; i < n; i++) {
        if (errors[i]) {
            lua_pushstring(L, errors[i]);
        } else {
            lua_pushnil(L);
        }
        free(errors[i]);
    }

    free(handles);
    free(errors);
    return 2 * n;
}

/*
 * H.query_sync(sql, params, opts?) -> result, err
 * H.altquery_sync(database_name, sql, params, opts?) -> result, err
 * H.authquery_sync(token, sql, params, opts?) -> result, err
 *
 * Convenience wrappers: submit + wait in one call. Return
 * (result, err) directly: result is a table on success or nil on
 * error; err is a string on error or nil on success.
 */
static int H_lua_query_sync(lua_State* L) {
    int n_pushed = H_lua_query(L);
    // H_lua_query pushed 1 handle. Now wait on it.
    if (n_pushed == 0) {
        lua_pushnil(L);
        lua_pushstring(L, "H.query_sync: handle allocation failed");
        return 2;
    }
    // The handle is at the top of the stack.
    H_Handle* h = H_Handle_check(L, -1);
    if (!h) {
        lua_pop(L, 1); // pop bad handle
        lua_pushnil(L);
        lua_pushstring(L, "H.query_sync: handle creation failed");
        return 2;
    }
    int n = H_lua_wait_one(L, h);
    // H_lua_wait_one pushed 2 values (result, err). The handle is
    // still on the stack below them. Pop the handle.
    lua_remove(L, -(n + 1));
    return n;
}

static int H_lua_altquery_sync(lua_State* L) {
    int n_pushed = H_lua_altquery(L);
    if (n_pushed == 0) {
        lua_pushnil(L);
        lua_pushstring(L, "H.altquery_sync: handle allocation failed");
        return 2;
    }
    H_Handle* h = H_Handle_check(L, -1);
    if (!h) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, "H.altquery_sync: handle creation failed");
        return 2;
    }
    int n = H_lua_wait_one(L, h);
    lua_remove(L, -(n + 1));
    return n;
}

static int H_lua_authquery_sync(lua_State* L) {
    int n_pushed = H_lua_authquery(L);
    if (n_pushed == 0) {
        lua_pushnil(L);
        lua_pushstring(L, "H.authquery_sync: handle allocation failed");
        return 2;
    }
    H_Handle* h = H_Handle_check(L, -1);
    if (!h) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, "H.authquery_sync: handle creation failed");
        return 2;
    }
    int n = H_lua_wait_one(L, h);
    lua_remove(L, -(n + 1));
    return n;
}

/*
 * Convert a Lua headers table (string -> string) into a libcurl
 * `struct curl_slist*` of "Name: Value" header lines. NULL is
 * returned when the argument is not a table or when the table is
 * empty (NULL is a valid slist for libcurl, meaning "no extra
 * headers beyond libcurl's default Accept").
 *
 * Phase 16. The caller is responsible for `curl_slist_free_all` on
 * the returned slist; the OIDC helper does this on its own
 * `_with_headers_slist` paths.
 */
static struct curl_slist* H_lua_headers_to_slist(lua_State* L, int idx) {
    if (!lua_istable(L, idx)) {
        return NULL;
    }
    struct curl_slist* list = NULL;

    // Push nil so the first lua_next returns the first key.
    lua_pushnil(L);
    while (lua_next(L, idx) != 0) {
        // Stack: ... key value
        const char* name = NULL;
        const char* value = NULL;
        size_t name_len = 0;
        size_t value_len = 0;

        if (lua_isstring(L, -2)) {
            name = lua_tolstring(L, -2, &name_len);
        }
        if (lua_isstring(L, -1)) {
            value = lua_tolstring(L, -1, &value_len);
        }
        if (name && value && name_len > 0) {
            // Build "Name: Value" header. libcurl tolerates long lines
            // but we cap at 8 KiB to avoid runaway allocations from
            // hostile scripts.
            size_t buf_len = name_len + value_len + 4; // ": " + NUL + slack
            if (buf_len > 8192) {
                log_this(SR_LUA,
                         "H_lua_headers_to_slist: header '%s' too long (%zu bytes), skipping",
                         LOG_LEVEL_ALERT, 2, name, name_len + value_len);
            } else {
                char* header = malloc(buf_len);
                if (header) {
                    // snprintf: "Name: Value\0"
                    int n = snprintf(header, buf_len, "%s: %s", name, value);
                    if (n > 0 && (size_t)n < buf_len) {
                        list = curl_slist_append(list, header);
                    }
                    free(header);
                }
            }
        } else {
            log_this(SR_LUA,
                     "H_lua_headers_to_slist: skipping non-string header (key or value)",
                     LOG_LEVEL_ALERT, 0);
        }
        // Pop value, keep key for next iteration.
        lua_pop(L, 1);
    }
    return list;
}

/*
 * Extract an integer `opts.timeout` from a Lua opts table at `idx`.
 * Returns -1 when the opts arg is not a table, missing, or has a
 * non-number timeout. Caller treats -1 as "use config default".
 */
static int H_lua_opts_timeout(lua_State* L, int idx) {
    if (!lua_istable(L, idx)) {
        return -1;
    }
    lua_getfield(L, idx, "timeout");
    int result = -1;
    if (lua_isnumber(L, -1)) {
        double v = lua_tonumber(L, -1);
        if (v > 0 && v <= (double)INT_MAX) {
            result = (int)v;
        }
    }
    lua_pop(L, 1);
    return result;
}

/*
 * H.http.get(url, headers?, opts?) -> handle
 *
 * Phase 16: returns an H_Handle of kind H_HK_HTTP. H.wait on the
 * handle performs the blocking libcurl call and pushes a result
 * table { status, headers, body, elapsed_ms }.
 *
 * url          string, required
 * headers      table of string->string, optional
 * opts.timeout number, optional (seconds; default = config)
 *
 * The "always return a handle" contract is preserved: any error
 * before the network call results in a handle whose `error` field
 * is set, so H.wait returns (nil, error) without crashing.
 */
static int H_lua_http_get(lua_State* L) {
    int nargs = lua_gettop(L);
    if (nargs < 1) {
        H_Handle* h = H_Handle_new(L, H_HK_HTTP);
        if (h) h->error = strdup("H.http.get: missing url argument");
        return 1;
    }
    const char* url = NULL;
    if (lua_isstring(L, 1)) {
        url = lua_tostring(L, 1);
    }
    if (!url || !*url) {
        H_Handle* h = H_Handle_new(L, H_HK_HTTP);
        if (h) h->error = strdup("H.http.get: url is not a non-empty string");
        return 1;
    }

    struct curl_slist* headers = NULL;
    if (nargs >= 2 && !lua_isnil(L, 2)) {
        headers = H_lua_headers_to_slist(L, 2);
    }

    int timeout = (nargs >= 3) ? H_lua_opts_timeout(L, 3) : -1;

    H_Handle* h = H_Handle_new(L, H_HK_HTTP);
    if (!h) {
        if (headers) curl_slist_free_all(headers);
        return 0;
    }
    h->http_url = strdup(url);
    h->http_method = strdup("GET");
    h->http_timeout = timeout;
    h->http_headers_slist = headers;  // ownership transferred to handle
    if (!h->http_url || !h->http_method) {
        h->error = strdup("H.http.get: allocation failed");
        return 1;
    }
    return 1;
}

/*
 * H.http.post(url, body?, headers?, opts?) -> handle
 *
 * body         string, optional (NULL = empty POST)
 * headers      table of string->string, optional
 * opts.timeout number, optional
 * opts.content_type  string, optional (e.g. "application/json").
 *                    Default: "application/octet-stream" when body is
 *                    non-NULL; omitted when body is NULL.
 */
static int H_lua_http_post(lua_State* L) {
    int nargs = lua_gettop(L);
    if (nargs < 1) {
        H_Handle* h = H_Handle_new(L, H_HK_HTTP);
        if (h) h->error = strdup("H.http.post: missing url argument");
        return 1;
    }
    const char* url = NULL;
    if (lua_isstring(L, 1)) {
        url = lua_tostring(L, 1);
    }
    if (!url || !*url) {
        H_Handle* h = H_Handle_new(L, H_HK_HTTP);
        if (h) h->error = strdup("H.http.post: url is not a non-empty string");
        return 1;
    }

    const char* body = NULL;
    int arg_idx = 2;
    if (nargs >= 2 && lua_isstring(L, 2)) {
        body = lua_tostring(L, 2);
        arg_idx = 3;
    } else if (nargs >= 2 && !lua_isnil(L, 2) && !lua_istable(L, 2)) {
        // Second arg is neither string nor nil nor table -> error
        H_Handle* h = H_Handle_new(L, H_HK_HTTP);
        if (h) h->error = strdup("H.http.post: body must be a string or nil");
        return 1;
    }

    struct curl_slist* headers = NULL;
    if (nargs >= arg_idx && lua_istable(L, arg_idx)) {
        headers = H_lua_headers_to_slist(L, arg_idx);
        arg_idx++;
    } else if (nargs >= arg_idx && !lua_isnil(L, arg_idx)) {
        H_Handle* h = H_Handle_new(L, H_HK_HTTP);
        if (h) h->error = strdup("H.http.post: headers must be a table or nil");
        return 1;
    } else if (nargs >= arg_idx) {
        arg_idx++;
    }

    int timeout = -1;
    const char* content_type = NULL;
    if (nargs >= arg_idx && lua_istable(L, arg_idx)) {
        timeout = H_lua_opts_timeout(L, arg_idx);
        lua_getfield(L, arg_idx, "content_type");
        if (lua_isstring(L, -1)) {
            content_type = lua_tostring(L, -1);
        }
        lua_pop(L, 1);
    }

    H_Handle* h = H_Handle_new(L, H_HK_HTTP);
    if (!h) {
        if (headers) curl_slist_free_all(headers);
        return 0;
    }
    h->http_url = strdup(url);
    h->http_method = strdup("POST");
    if (body) h->http_body = strdup(body);
    if (content_type) h->http_content_type = strdup(content_type);
    h->http_timeout = timeout;
    h->http_headers_slist = headers;  // ownership transferred to handle
    if (!h->http_url || !h->http_method) {
        h->error = strdup("H.http.post: allocation failed");
        return 1;
    }
    return 1;
}

/*
 * H_lua_http_wait_one: perform the HTTP call for one handle and
 * push the result/error pair. Mirrors the structure of
 * H_lua_wait_one for H_HK_QUERY.
 *
 * Phase 16: performed the blocking libcurl call on the calling
 * thread. Phase 17: submits the handle to the HTTP worker pool
 * and blocks on the handle's per-handle condvar until a worker
 * stores the result. This lets multiple H.http.get calls fan out
 * in parallel.
 *
 * Returns 2 (one result + one error).
 */
static int H_lua_http_wait_one(lua_State* L, H_Handle* h) {
    if (!h) {
        lua_pushnil(L);
        lua_pushstring(L, "H.wait: invalid handle");
        return 2;
    }
    if (h->consumed) {
        lua_pushnil(L);
        lua_pushstring(L, "H.wait: handle already consumed");
        return 2;
    }
    if (h->error) {
        lua_pushnil(L);
        lua_pushstring(L, h->error);
        h->consumed = true;
        return 2;
    }
    if (!h->http_url || !h->http_method) {
        lua_pushnil(L);
        lua_pushstring(L, "H.wait: HTTP handle missing url or method");
        h->consumed = true;
        return 2;
    }

    // Phase 17: submit the handle to the HTTP worker pool. The
    // pool acquires a refcount on the handle so it cannot be freed
    // while the call is in flight. If the pool is not running
    // (e.g. scripting is disabled but Lua still holds a handle),
    // fall back to a synchronous inline call so the handle still
    // resolves. The fallback matches the Phase 16 behavior.
    if (!scripting_http_pool_submit(h)) {
        // Fallback: perform the call inline (Phase 16 path).
        // This branch is hit when scripting is disabled or the
        // pool is not initialized. The test seam in http_client.c
        // is what unit tests use, so the fallback is not exercised
        // in tests.
        struct curl_slist* headers = (struct curl_slist*)h->http_headers_slist;
        h->http_headers_slist = NULL;

        struct timespec start_ts;
        clock_gettime(CLOCK_MONOTONIC, &start_ts);

        struct OidcRpHttpResponse* resp;
        if (strcmp(h->http_method, "GET") == 0) {
            resp = scripting_http_get(h->http_url, headers,
                                      h->http_timeout, true);
        } else if (strcmp(h->http_method, "POST") == 0) {
            resp = scripting_http_post(h->http_url, h->http_body,
                                       h->http_content_type, headers,
                                       h->http_timeout, true);
        } else {
            if (headers) curl_slist_free_all(headers);
            lua_pushnil(L);
            lua_pushstring(L, "H.wait: unknown HTTP method on handle");
            h->consumed = true;
            return 2;
        }

        h->consumed = true;

        if (!resp) {
            lua_pushnil(L);
            lua_pushstring(L, "H.wait: HTTP call returned no response");
            return 2;
        }

        struct timespec end_ts;
        clock_gettime(CLOCK_MONOTONIC, &end_ts);
        long elapsed_ms = (long)(((end_ts.tv_sec - start_ts.tv_sec) * 1000L) +
                                 ((end_ts.tv_nsec - start_ts.tv_nsec) / 1000000L));

        if (resp->error_message) {
            char buf[512];
            snprintf(buf, sizeof(buf), "H.wait: %s", resp->error_message);
            lua_pushnil(L);
            lua_pushstring(L, buf);
            oidc_rp_http_response_free(resp);
            return 2;
        }

        lua_createtable(L, 0, 4);
        lua_pushinteger(L, (lua_Integer)resp->http_status);
        lua_setfield(L, -2, "status");

        lua_createtable(L, 0, (int)resp->headers_count);
        for (size_t i = 0; i < resp->headers_count; i++) {
            const char* name = resp->headers[i].name ? resp->headers[i].name : "";
            const char* value = resp->headers[i].value ? resp->headers[i].value : "";
            lua_getfield(L, -1, name);
            if (lua_isnil(L, -1)) {
                lua_pop(L, 1);
                lua_pushstring(L, value);
                lua_setfield(L, -2, name);
            } else {
                size_t existing_len = 0;
                const char* existing = lua_tolstring(L, -1, &existing_len);
                size_t total = existing_len + strlen(value) + 3;
                char* combined = malloc(total);
                if (combined) {
                    snprintf(combined, total, "%s, %s", existing, value);
                    lua_pop(L, 1);
                    lua_pushstring(L, combined);
                    lua_setfield(L, -2, name);
                    free(combined);
                } else {
                    lua_pop(L, 1);
                }
            }
        }
        lua_setfield(L, -2, "headers");

        if (resp->body) {
            lua_pushstring(L, resp->body);
        } else {
            lua_pushliteral(L, "");
        }
        lua_setfield(L, -2, "body");

        lua_pushinteger(L, (lua_Integer)elapsed_ms);
        lua_setfield(L, -2, "elapsed_ms");

        lua_pushnil(L);

        oidc_rp_http_response_free(resp);
        return 2;
    }

    // Pool path: block on the handle's condvar until a worker
    // stores the result.
    pthread_mutex_lock(&h->http_mutex);
    while (!h->http_ready) {
        pthread_cond_wait(&h->http_cond, &h->http_mutex);
    }

    // Snapshot the result fields under the lock, then unlock
    // before doing the Lua table building (which may call back
    // into the runtime).
    long elapsed_ms = h->http_result_elapsed_ms;
    int status = h->http_result_status;
    char* body_copy = h->http_result_body ? strdup(h->http_result_body) : NULL;
    char* headers_json_copy = h->http_result_headers_json
        ? strdup(h->http_result_headers_json) : NULL;
    char* error_copy = h->http_result_error ? strdup(h->http_result_error) : NULL;

    pthread_mutex_unlock(&h->http_mutex);

    h->consumed = true;

    if (error_copy) {
        char buf[512];
        snprintf(buf, sizeof(buf), "H.wait: %s", error_copy);
        lua_pushnil(L);
        lua_pushstring(L, buf);
        free(error_copy);
        free(body_copy);
        free(headers_json_copy);
        return 2;
    }

    lua_createtable(L, 0, 4);
    lua_pushinteger(L, (lua_Integer)status);
    lua_setfield(L, -2, "status");

    // Headers table: parse the JSON array of {name, value} pairs
    // and build a Lua table with case-insensitive insert (RFC 7230
    // multi-value collapse).
    lua_createtable(L, 0, 0);
    if (headers_json_copy) {
        json_error_t jerr;
        json_t* arr = json_loads(headers_json_copy, 0, &jerr);
        if (arr && json_is_array(arr)) {
            size_t i;
            json_t* obj;
            json_array_foreach(arr, i, obj) {
                const char* name = NULL;
                const char* value = NULL;
                json_t* jname = json_object_get(obj, "name");
                json_t* jval = json_object_get(obj, "value");
                if (json_is_string(jname)) name = json_string_value(jname);
                if (json_is_string(jval))  value = json_string_value(jval);
                if (!name) name = "";
                if (!value) value = "";
                lua_getfield(L, -1, name);
                if (lua_isnil(L, -1)) {
                    lua_pop(L, 1);
                    lua_pushstring(L, value);
                    lua_setfield(L, -2, name);
                } else {
                    size_t existing_len = 0;
                    const char* existing = lua_tolstring(L, -1, &existing_len);
                    size_t total = existing_len + strlen(value) + 3;
                    char* combined = malloc(total);
                    if (combined) {
                        snprintf(combined, total, "%s, %s", existing, value);
                        lua_pop(L, 1);
                        lua_pushstring(L, combined);
                        lua_setfield(L, -2, name);
                        free(combined);
                    } else {
                        lua_pop(L, 1);
                    }
                }
            }
        }
        if (arr) json_decref(arr);
    }
    lua_setfield(L, -2, "headers");

    if (body_copy) {
        lua_pushstring(L, body_copy);
    } else {
        lua_pushliteral(L, "");
    }
    lua_setfield(L, -2, "body");

    lua_pushinteger(L, (lua_Integer)elapsed_ms);
    lua_setfield(L, -2, "elapsed_ms");

    lua_pushnil(L);

    free(body_copy);
    free(headers_json_copy);
    return 2;
}

/*
 * H.http.get_sync(url, headers?, opts?) -> result, err
 * H.http.post_sync(url, body?, headers?, opts?) -> result, err
 *
 * Convenience wrappers: submit + wait in one call. Return
 * (result, err) directly.
 */
static int H_lua_http_get_sync(lua_State* L) {
    int n_pushed = H_lua_http_get(L);
    if (n_pushed == 0) {
        lua_pushnil(L);
        lua_pushstring(L, "H.http.get_sync: handle allocation failed");
        return 2;
    }
    H_Handle* h = H_Handle_check(L, -1);
    if (!h) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, "H.http.get_sync: handle creation failed");
        return 2;
    }
    int n = H_lua_http_wait_one(L, h);
    lua_remove(L, -(n + 1));
    return n;
}

static int H_lua_http_post_sync(lua_State* L) {
    int n_pushed = H_lua_http_post(L);
    if (n_pushed == 0) {
        lua_pushnil(L);
        lua_pushstring(L, "H.http.post_sync: handle allocation failed");
        return 2;
    }
    H_Handle* h = H_Handle_check(L, -1);
    if (!h) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, "H.http.post_sync: handle creation failed");
        return 2;
    }
    int n = H_lua_http_wait_one(L, h);
    lua_remove(L, -(n + 1));
    return n;
}

/*
 * Populate H.http with the four H.http functions and add a
 * sub-table of HTTP-related helpers. Replaces the Phase 3
 * placeholder sub-table of the same name.
 */
void H_lua_install_http(lua_State* L) {
    if (!L) return;

    // The H.Handle metatable must be installed before any handle
    // can be created. H_lua_install_query does this; calling
    // install_query first (or this) is idempotent.
    H_Handle_install_metatable(L);

    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_http: H table missing",
                 LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }

    // H.http is a sub-table. Phase 3 left a placeholder sub-table
    // here; we replace it with a fresh sub-table that holds the
    // four real functions.
    lua_newtable(L);
    lua_pushcfunction(L, H_lua_http_get);        lua_setfield(L, -2, "get");
    lua_pushcfunction(L, H_lua_http_post);       lua_setfield(L, -2, "post");
    lua_pushcfunction(L, H_lua_http_get_sync);   lua_setfield(L, -2, "get_sync");
    lua_pushcfunction(L, H_lua_http_post_sync);  lua_setfield(L, -2, "post_sync");
    lua_setfield(L, -2, "http");

    lua_pop(L, 1); // pop H
}
void H_lua_install_query(lua_State* L) {
    if (!L) return;

    // Install the H.Handle metatable first (needed for handle creation).
    H_Handle_install_metatable(L);

    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_query: H table missing",
                 LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }

    // H.query, H.altquery, H.authquery, H.wait are top-level functions
    // on H (replacing the Phase 3 placeholder sub-tables of the same names).
    lua_pushcfunction(L, H_lua_query);        lua_setfield(L, -2, "query");
    lua_pushcfunction(L, H_lua_altquery);     lua_setfield(L, -2, "altquery");
    lua_pushcfunction(L, H_lua_authquery);    lua_setfield(L, -2, "authquery");
    lua_pushcfunction(L, H_lua_wait);         lua_setfield(L, -2, "wait");

    // Sync wrappers as additional top-level functions.
    lua_pushcfunction(L, H_lua_query_sync);        lua_setfield(L, -2, "query_sync");
    lua_pushcfunction(L, H_lua_altquery_sync);     lua_setfield(L, -2, "altquery_sync");
    lua_pushcfunction(L, H_lua_authquery_sync);    lua_setfield(L, -2, "authquery_sync");

    // H.http sub-table (Phase 16). The functions are installed as
    // a sub-table to leave room for future helpers (e.g. an
    // async-multiplexer in Phase 17).
    H_lua_install_http(L);

    lua_pop(L, 1); // pop H
}
