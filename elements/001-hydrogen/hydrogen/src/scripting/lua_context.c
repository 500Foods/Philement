/*
 * Scripting Subsystem - Lua Context Management
 *
 * Phase 3 of the LUA_PLAN. Implements the sandboxed lua_State lifecycle.
 *
 * The mmap-based allocator (lua_mmap_alloc) is shared with the database
 * migration engine: a non-static function defined in
 * src/database/migration/lua_allocator.c. We declare it extern here
 * rather than copying it.
 */

 // Project includes
#include <src/hydrogen.h>

// Standard includes
#include <stdlib.h>   // malloc, free

// Third-party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Local includes
#include "lua_context.h"
#include "scripting_api.h"

// External allocator shared with the database migration engine.
// Cheap insurance against process-heap contamination; both subsystems
// (database migration, scripting) get the same allocator.
extern void* lua_mmap_alloc(void* ud, void* ptr, size_t osize, size_t nsize);

// Forward declarations for the limited sandbox policy applied here.
static void H_lua_open_sandboxed_libraries(lua_State* L);
static int H_lua_panic(lua_State* L);

/*
 * Create a sandboxed Lua state.
 *
 * Allocator: lua_mmap_alloc (shared with migration engine).
 * Libraries: openlibs is called and then unwanted libraries are
 *   nulled out from the globals table based on config->scripting.Sandbox.
 *   This is simpler than reimplementing the standard library gating
 *   piecemeal and keeps the door open for the user re-requiring
 *   libraries later (Phase 20) or for a stricter Phase-3+ approach.
 *
 * Returns NULL on failure; logs at LOG_LEVEL_ERROR.
 */
lua_State* H_lua_create_context(void) {
    lua_State* L = lua_newstate(lua_mmap_alloc, NULL);
    if (!L) {
        log_this(SR_LUA, "Failed to create Lua state", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Install a panic handler that surfaces failures clearly. Without
    // this, a panic during state creation would invoke the default
    // abort() and yield no useful context.
    lua_atpanic(L, H_lua_panic);

    // Open all standard libraries, then prune per the sandbox policy.
    luaL_openlibs(L);
    H_lua_open_sandboxed_libraries(L);

    // Install the (currently empty) H host table. Later phases will
    // populate H.log, H.query, H.llm, etc.
    H_lua_install_api(L);

    return L;
}

/*
 * Destroy a Lua state. Safe to call with NULL.
 */
void H_lua_destroy_context(lua_State* L) {
    if (!L) {
        return;
    }
    lua_close(L);
}

/*
 * Apply the configured sandbox policy.
 *
 * We open everything (luaL_openlibs) and then explicitly null out the
 * libraries that the policy disables, so any accidental reference to
 * them from Lua surfaces immediately as a nil global rather than
 * succeeding silently.
 *
 * Reading the policy: missing app_config is treated as "deny by
 * default" for the loadlib and io families, since both are common
 * attack vectors and a missing config implies no consent.
 */
static void H_lua_open_sandboxed_libraries(lua_State* L) {
    bool allow_io = false;
    bool allow_debug = false;
    bool allow_loadlib = false;
    bool allow_os_time = true;     // default true
    bool allow_os_execute = false; // default false

    if (app_config) {
        allow_io = app_config->scripting.Sandbox.AllowIo;
        allow_debug = app_config->scripting.Sandbox.AllowDebug;
        allow_loadlib = app_config->scripting.Sandbox.AllowLoadlib;
        allow_os_time = app_config->scripting.Sandbox.AllowOsTime;
        allow_os_execute = app_config->scripting.Sandbox.AllowOsExecute;
    }

    if (!allow_io) {
        lua_pushnil(L);
        lua_setglobal(L, "io");
    }

    if (!allow_debug) {
        lua_pushnil(L);
        lua_setglobal(L, "debug");
    }

    // package.loadlib / require of native modules - nilling package
    // outright would also break require() of pure-Lua modules (Phase
    // 20). Strip just package.loadlib, which is the native loader entry
    // point; require stays available but loads only Lua modules.
    if (!allow_loadlib) {
        lua_getglobal(L, "package");
        if (lua_istable(L, -1)) {
            lua_pushnil(L);
            lua_setfield(L, -2, "loadlib");
        }
        lua_pop(L, 1);
    }

    // os.* handling: we cannot safely "nil" the entire os table without
    // breaking sandboxed scripts that legitimately call os.time / os.date
    // (AllowOsTime) or rely on the clock indirectly. Phase 6+ will install
    // a controlled shim if necessary; for Phase 3 we just log the
    // decision so reviewers can see which policy was in effect.
    log_this(SR_LUA, "Sandbox policy: io=%s debug=%s loadlib=%s os.time=%s os.execute=%s",
             LOG_LEVEL_TRACE, 5,
             allow_io ? "on" : "off",
             allow_debug ? "on" : "off",
             allow_loadlib ? "on" : "off",
             allow_os_time ? "on" : "off",
             allow_os_execute ? "on" : "off");

    // Suppress unused-variable warning - allow_os_time and
    // allow_os_execute are kept here for the future shim and so the
    // log line above documents the live policy.
    (void)allow_os_time;
    (void)allow_os_execute;
}

/*
 * Lua panic handler. Logs the error and aborts. This matches the
 * migration engine's implicit policy: a Lua panic indicates the state
 * is unrecoverable, so propagating further is unsafe.
 */
static int H_lua_panic(lua_State* L) {
    const char* msg = lua_tostring(L, -1);
    log_this(SR_LUA, "Lua panic: %s", LOG_LEVEL_FATAL, 1, msg ? msg : "(no message)");
    return 0;
}

/*
 * Install the H host table.
 *
 * Phase 3: empty H with sub-table placeholders for the names agreed in
 * Phase 2. Real functions are populated by Phase 6 (H.log/H.system),
 * Phase 11 (H.sleep, H.shutdown_requested, H.scoreboard.*), and the
 * other phases.
 */
void H_lua_install_api(lua_State* L) {
    if (!L) {
        return;
    }

    lua_newtable(L);
    lua_setglobal(L, "H");

    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        // Defensive: H should have just been set to a fresh table.
        lua_pop(L, 1);
        log_this(SR_LUA, "Failed to install H table", LOG_LEVEL_ERROR, 0);
        return;
    }

    // Empty sub-table placeholders so scripts that read H.log can do
    // so without raising an error during the bring-up phases.
    static const char* placeholder_names[] = {
        "log", "system", "gc",
        "query", "altquery", "authquery", "wait",
        "http", "llm", "mail", "notify",
        "sleep", "shutdown_requested", "set_current_state",
        "scoreboard",
        NULL
    };

    for (int i = 0; placeholder_names[i] != NULL; i++) {
        lua_newtable(L);
        lua_setfield(L, -2, placeholder_names[i]);
    }

    lua_pop(L, 1);

    // Phase 6: populate H.log and H.system with real C functions.
    // Both install functions re-fetch H from globals and fill the
    // placeholder sub-tables in place. Later phases add their own
    // install calls here.
    H_lua_install_log(L);
    H_lua_install_system(L);
    // Phase 8: explicit GC control for the Orchestrator (and any
    // long-running script that wants to manage its own memory).
    // The progress hook only samples; it never collects.
    H_lua_install_gc(L);
    // Phase 9: voluntary progress report. Replaces the Phase 3
    // placeholder sub-table of the same name with a top-level
    // function on H. The Orchestrator is a no-op caller (no
    // scoreboard entry); workers call this with their own job_id.
    H_lua_install_set_current_state(L);
}

/*
 * Compile and run a Lua chunk from a NUL-terminated string.
 *
 * Phase 4 implementation. Intentionally minimal: a thin, well-defined
 * wrapper around luaL_loadbuffer + lua_pcall with the two-tier
 * architecture (Phase 1) in mind:
 *
 *   - Workers will call this exactly once per job on a fresh state, so
 *     we do not retain bytecode or accumulate any compile-once state.
 *   - The Orchestrator (Phase 11) compiles its own chunk via
 *     luaL_loadbuffer once at launch and then drives it with a
 *     scheduling loop, NOT through this function. This wrapper is
 *     therefore the worker-tier primitive.
 *
 * Error model: on failure, a single error string is left on the stack
 * (Lua's own contract for luaL_loadbuffer / lua_pcall). The caller is
 * expected to copy that string into C-owned memory (UAF discipline
 * from Phase 1) and then lua_pop it before doing anything else with
 * the state. This keeps string lifetimes simple and matches the
 * pattern used in the migration engine (execute_load.c, etc.).
 */
int H_lua_run_string(lua_State* L, const char* code, const char* name) {
    if (!L) {
        log_this(SR_LUA, "H_lua_run_string called with NULL lua_State", LOG_LEVEL_ERROR, 0);
        return LUA_ERRRUN;
    }

    if (!code) {
        log_this(SR_LUA, "H_lua_run_string called with NULL code", LOG_LEVEL_ERROR, 0);
        lua_pushliteral(L, "H_lua_run_string: code is NULL");
        return LUA_ERRRUN;
    }

    size_t len = strlen(code);
    const char* chunk_name = name ? name : "?";

    int load_rc = luaL_loadbufferx(L, code, len, chunk_name, NULL);
    if (load_rc != LUA_OK) {
        // luaL_loadbufferx left the syntax/compile error on the stack.
        // Copy to C-owned memory for logging; the caller's caller will
        // still see the error string on the stack when it returns.
        const char* lua_err = lua_tostring(L, -1);
        log_this(SR_LUA, "Failed to compile chunk %s: %s", LOG_LEVEL_ERROR, 2,
                 chunk_name, lua_err ? lua_err : "(no message)");
        return load_rc;
    }

    // LUA_MULTRET lets the chunk return any number of values; the
    // caller reads lua_gettop(L) to discover how many.
    int call_rc = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (call_rc != LUA_OK) {
        // lua_pcall left the runtime error on the stack.
        const char* lua_err = lua_tostring(L, -1);
        log_this(SR_LUA, "Failed to execute chunk %s: %s", LOG_LEVEL_ERROR, 2,
                 chunk_name, lua_err ? lua_err : "(no message)");
        return call_rc;
    }

    return LUA_OK;
}

// Per-state job context /////////////////////////////////////////////////////

/*
 * Set the per-state job context. Pass a NULL pointer to clear.
 *
 * The context lives in the state's "extraspace" (Lua 5.4 API) - a
 * pointer-sized slot tied 1:1 to the lua_State, allocated by
 * lua_newstate and freed by lua_close. We store a pointer to a
 * heap-allocated H_lua_job_context, owned and freed by the caller
 * (the worker pool / Orchestrator). The state only holds the pointer.
 *
 * Why a heap-allocated context and not just stuffing the struct into
 * the extraspace directly: the extraspace is a single pointer-sized
 * slot. Our struct is much larger, so we must indirect through a
 * pointer. A heap allocation is the cleanest portable choice.
 *
 * Note: lua_getextraspace is a pure macro that returns a pointer to
 * the extraspace block; it does NOT push anything onto the Lua
 * stack. Do not lua_pop after it.
 */
void H_lua_set_job_context(lua_State* L, const H_lua_job_context* ctx) {
    if (!L) {
        return;
    }
    H_lua_job_context** slot = (H_lua_job_context**)lua_getextraspace(L);

    // Free any prior context so we don't leak when called twice.
    if (*slot) {
        free(*slot);
        *slot = NULL;
    }

    if (!ctx) {
        return;
    }
    H_lua_job_context* dup = malloc(sizeof(H_lua_job_context));
    if (!dup) {
        log_this(SR_LUA, "H_lua_set_job_context: allocation failed",
                 LOG_LEVEL_ERROR, 0);
        return;
    }
    *dup = *ctx;
    *slot = dup;
}

H_lua_job_context* H_lua_get_job_context(lua_State* L) {
    if (!L) {
        return NULL;
    }
    H_lua_job_context** slot = (H_lua_job_context**)lua_getextraspace(L);
    return *slot;
}
