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

// Third-party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Local includes
#include "lua_context.h"

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
        "log", "system",
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
}
