/*
 * Scripting Subsystem - Host API: H.log
 *
 * C-side implementations of H.log.{trace,debug,info,warn,error,fatal}.
 */

// Project includes
#include <src/hydrogen.h>

// Third-party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Local includes
#include "scripting_api.h"
#include "scripting_api_internal.h"

// H.log.* implementations //////////////////////////////////////////////////

/*
 * H.log.<level>(fmt, ...) - format fmt with the trailing args and emit
 * a log line at the given priority, tagged with the Scripting subsystem.
 */
int H_lua_log_at_level(lua_State* L, int priority) {
    int nargs = lua_gettop(L);
    if (nargs < 1) {
        log_this(SR_SCRIPTING, "H.log: missing format string", LOG_LEVEL_ERROR, 0);
        return 0;
    }

    if (!lua_isstring(L, 1)) {
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
    lua_remove(L, -2);

    for (int i = 1; i <= nargs; i++) {
        lua_pushvalue(L, i);
    }

    if (lua_pcall(L, nargs, 1, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        log_this(SR_SCRIPTING, "H.log: string.format failed: %s",
                 LOG_LEVEL_ERROR, 1, err ? err : "(no message)");
        lua_pop(L, 1);
        return 0;
    }

    const char* msg = lua_tostring(L, -1);
    if (msg) {
        log_this(SR_SCRIPTING, msg, priority, 0);
    } else {
        log_this(SR_SCRIPTING, "H.log: formatted message is not a string",
                 LOG_LEVEL_ERROR, 0);
    }
    lua_pop(L, 1);

    return 0;
}

int H_lua_log_trace(lua_State* L) { return H_lua_log_at_level(L, LOG_LEVEL_TRACE); }
int H_lua_log_debug(lua_State* L) { return H_lua_log_at_level(L, LOG_LEVEL_DEBUG); }
int H_lua_log_info(lua_State* L)  { return H_lua_log_at_level(L, LOG_LEVEL_STATE); }
int H_lua_log_warn(lua_State* L)  { return H_lua_log_at_level(L, LOG_LEVEL_ALERT); }
int H_lua_log_error(lua_State* L) { return H_lua_log_at_level(L, LOG_LEVEL_ERROR); }
int H_lua_log_fatal(lua_State* L) { return H_lua_log_at_level(L, LOG_LEVEL_FATAL); }

// Install functions ////////////////////////////////////////////////////////

/*
 * Populate H.log with the six level functions.
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

    lua_pop(L, 2);
}
