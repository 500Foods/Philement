/*
 * Scripting Subsystem - Host API: H.scoreboard and H.package.searcher
 *
 * C-side implementations of the scoreboard access functions and the
 * DB-backed package searcher.
 */

// Project includes
#include <src/hydrogen.h>

// System includes
#include <stdlib.h>
#include <string.h>

// Third-party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Local includes
#include "scripting_api.h"
#include "scripting_api_internal.h"
#include "lua_context.h"
#include "scoreboard.h"
#include "scripting.h"
#include "scripting_handle.h"
#include "source_cache.h"
#include "orchestrator.h"
#include "worker_pool.h"

// H.scoreboard.* implementations //////////////////////////////////////////

Scoreboard* resolve_active_scoreboard(lua_State* L) {
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
void push_scoreboard_entry_as_table(lua_State* L, const ScoreboardEntry* e) {
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

int H_lua_scoreboard_list(lua_State* L) {
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

int H_lua_scoreboard_get(lua_State* L) {
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

int H_lua_scoreboard_submit(lua_State* L) {
    int nargs = lua_gettop(L);
    if (nargs < 1 || !lua_istable(L, 1)) {
        log_this(SR_SCRIPTING,
                 "H.scoreboard.submit: entry must be a table",
                 LOG_LEVEL_ERROR, 0);
        lua_pushnil(L);
        return 1;
    }
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

    char* params_json = NULL;
    lua_getfield(L, 1, "params_json");
    if (lua_isstring(L, -1)) {
        const char* p = lua_tostring(L, -1);
        if (p) {
            params_json = strdup(p);
        }
    }
    lua_pop(L, 1);

    char* source = NULL;
    lua_getfield(L, 1, "source");
    if (lua_isstring(L, -1)) {
        const char* s = lua_tostring(L, -1);
        if (s) {
            source = strdup(s);
        }
    }
    lua_pop(L, 1);

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

int H_lua_scoreboard_cancel(lua_State* L) {
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

// H.package.searcher implementation (Phase 11g) ////////////////////////////

// Bytecode dump buffer for caching compiled modules
typedef struct {
    void*   data;
    size_t  len;
    size_t  capacity;
} BytecodeDumpBuffer;

static int bytecode_dump_writer(lua_State* L, const void* p, size_t sz, void* ud) {
    (void)L;
    BytecodeDumpBuffer* buf = (BytecodeDumpBuffer*)ud;
    if (buf->len + sz > buf->capacity) {
        size_t new_capacity = buf->capacity ? buf->capacity * 2 : 256;
        while (new_capacity < buf->len + sz) {
            new_capacity *= 2;
        }
        void* new_data = realloc(buf->data, new_capacity);
        if (!new_data) {
            return 1;  // Error: out of memory
        }
        buf->data = new_data;
        buf->capacity = new_capacity;
    }
    memcpy((char*)buf->data + buf->len, p, sz);
    buf->len += sz;
    return 0;
}

bool split_module_name(const char* name,
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

int H_lua_package_searcher(lua_State* L) {
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

    // Phase 21: Check for cached bytecode first
    size_t bytecode_len = 0;
    const void* bytecode = source_cache_get_bytecode(
        scripting_source_cache, group, script, &bytecode_len);

    if (bytecode && bytecode_len > 0) {
        // Load from cached bytecode (mode "b" = bytecode only)
        int rc = luaL_loadbufferx(L, bytecode, bytecode_len, module_name, "b");

        free(group);
        free(script);

        if (rc != LUA_OK) {
            return 1;
        }
        return 1;
    }

    // No cached bytecode - load from source and compile
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
    int rc = luaL_loadbufferx(L, source, source_len, module_name, "bt");

    if (rc == LUA_OK) {
        // Phase 21: Dump compiled bytecode to cache for future use
        BytecodeDumpBuffer dump = {0, 0, 0};
        if (lua_dump(L, bytecode_dump_writer, &dump, 0) == 0) {
            if (dump.data && dump.len > 0) {
                source_cache_put_bytecode(
                    scripting_source_cache, group, script, dump.data, dump.len);
            }
        }
        free(dump.data);
    }

    free(group);
    free(script);
    if (fetched_source) {
        free(fetched_source);
    }

    if (rc != LUA_OK) {
        return 1;
    }

    return 1;
}

// Install functions ////////////////////////////////////////////////////////

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

    lua_pop(L, 2);
}

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

    lua_pop(L, 2);
}
