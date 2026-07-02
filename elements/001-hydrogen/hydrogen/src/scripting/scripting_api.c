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
