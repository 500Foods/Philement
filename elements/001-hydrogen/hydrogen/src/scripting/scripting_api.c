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

// Third-party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Local includes
#include "scripting_api.h"
#include "lua_context.h"   // H_lua_job_context, H_lua_get_job_context
#include "scoreboard.h"     // scoreboard_update_current_state
#include "scripting.h"      // scripting_system_shutdown, scripting_scoreboard
#include "source_cache.h"   // source_cache_get/put
#include "orchestrator.h"   // scripting_fetch_script_source
#include "worker_pool.h"    // scripting_submit_job_with_source

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
