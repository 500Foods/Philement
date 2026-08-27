#include <src/hydrogen.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <lua.h>
#include <lauxlib.h>
#include <jansson.h>

#include "scripting_api.h"
#include "scripting_api_internal.h"
#include "scripting_handle.h"
#include "lua_context.h"
#include "orchestrator.h"
#include "scripting_invoke.h"
#include "worker_pool.h"
#include "scoreboard.h"

#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database_pending.h>
#include <src/database/database.h>
#include <src/api/conduit/conduit_helpers.h>

#define H_SCRIPTING_MCP_LIST_QUERYREF 152
#define H_MCP_DEFAULT_PAGE_SIZE 50
#define H_MCP_MAX_PAGE_SIZE 500
#define H_MCP_LIST_TIMEOUT_SECONDS 30

H_lua_mcp_list_rows_fn H_lua_mcp_list_rows_hook = NULL;
H_lua_mcp_fetch_source_fn H_lua_mcp_fetch_source_hook = NULL;
H_lua_mcp_submit_job_fn H_lua_mcp_submit_job_hook = NULL;
H_lua_mcp_wait_job_fn H_lua_mcp_wait_job_hook = NULL;
int H_lua_mcp_submit_count = 0;

char H_lua_mcp_inline_result_key;

void H_lua_mcp_clear_hooks(void) {
    H_lua_mcp_list_rows_hook = NULL;
    H_lua_mcp_fetch_source_hook = NULL;
    H_lua_mcp_submit_job_hook = NULL;
    H_lua_mcp_wait_job_hook = NULL;
    H_lua_mcp_submit_count = 0;
}

int H_lua_mcp_capture_result_json(lua_State* L) {
    char* json;

    if (!L || !lua_istable(L, 1)) {
        return 0;
    }
    json = H_lua_table_to_json_string(L, 1);
    if (!json) {
        return 0;
    }
    lua_pushlightuserdata(L, (void*)&H_lua_mcp_inline_result_key);
    lua_pushstring(L, json);
    lua_settable(L, LUA_REGISTRYINDEX);
    free(json);
    return 0;
}

char* H_lua_mcp_fetch_list_rows_json(void) {
    const char* database;
    DatabaseQueue* db_queue;
    QueryCacheEntry* cache_entry;
    char* query_id;
    char* params_json;
    PendingResultManager* pending_mgr;
    PendingQueryResult* pending;
    QueryResult* query_result;
    char* data_copy;
    DatabaseQuery db_query;
    int wait_result;

    if (H_lua_mcp_list_rows_hook) {
        return H_lua_mcp_list_rows_hook();
    }

    database = orchestrator_resolve_database();
    if (!database || database[0] == '\0') {
        return NULL;
    }
    db_queue = database_queue_manager_get_database(global_queue_manager, database);
    if (!db_queue) {
        return NULL;
    }
    cache_entry = lookup_query_cache_entry(db_queue, H_SCRIPTING_MCP_LIST_QUERYREF);
    if (!cache_entry) {
        log_this(SR_SCRIPTING,
                 "H.mcp.list: QueryRef %d not found in cache",
                 LOG_LEVEL_ERROR, 1, H_SCRIPTING_MCP_LIST_QUERYREF);
        return NULL;
    }
    query_id = generate_query_id();
    if (!query_id) {
        return NULL;
    }
    params_json = strdup("{}");
    if (!params_json) {
        free(query_id);
        return NULL;
    }
    db_query.query_id = query_id;
    db_query.query_template = strdup(cache_entry->sql_template);
    db_query.parameter_json = params_json;
    db_query.queue_type_hint = database_queue_type_from_string(cache_entry->queue_type);
    db_query.submitted_at = time(NULL);
    db_query.processed_at = 0;
    db_query.retry_count = 0;
    db_query.error_message = NULL;
    if (!db_query.query_template) {
        free(query_id);
        free(params_json);
        return NULL;
    }
    pending_mgr = get_pending_result_manager();
    if (!pending_mgr) {
        free(query_id);
        free(db_query.query_template);
        free(params_json);
        return NULL;
    }
    pending = pending_result_register(pending_mgr, query_id,
                                      H_MCP_LIST_TIMEOUT_SECONDS, SR_SCRIPTING);
    if (!pending) {
        free(query_id);
        free(db_query.query_template);
        free(params_json);
        return NULL;
    }
    if (!database_queue_submit_query(db_queue, &db_query)) {
        pending_result_unregister(pending_mgr, pending, SR_SCRIPTING);
        free(query_id);
        free(db_query.query_template);
        free(params_json);
        return NULL;
    }
    wait_result = pending_result_wait(pending, SR_SCRIPTING);
    if (wait_result != 0) {
        pending_result_unregister(pending_mgr, pending, SR_SCRIPTING);
        free(query_id);
        free(db_query.query_template);
        free(params_json);
        return NULL;
    }
    query_result = pending_result_get(pending);
    data_copy = NULL;
    if (query_result && query_result->data_json) {
        data_copy = strdup(query_result->data_json);
    }
    pending_result_unregister(pending_mgr, pending, SR_SCRIPTING);
    free(query_id);
    free(db_query.query_template);
    free(params_json);
    return data_copy;
}

void H_lua_mcp_push_decoded_json_field(lua_State* L, json_t* row, const char* key) {
    json_t* field;
    json_error_t err;
    json_t* parsed;

    field = json_object_get(row, key);
    if (!field || json_is_null(field)) {
        lua_pushnil(L);
        return;
    }
    if (json_is_object(field) || json_is_array(field)) {
        push_json_value_as_lua(L, field);
        return;
    }
    if (!json_is_string(field)) {
        lua_pushnil(L);
        return;
    }
    parsed = json_loads(json_string_value(field), 0, &err);
    if (!parsed) {
        lua_pushnil(L);
        return;
    }
    push_json_value_as_lua(L, parsed);
    json_decref(parsed);
}

void H_lua_mcp_push_row(lua_State* L, json_t* row) {
    const char* group;
    const char* script;
    const char* summary;
    json_t* j;

    lua_newtable(L);
    group = NULL;
    script = NULL;
    summary = NULL;
    j = json_object_get(row, "group_name");
    if (j && json_is_string(j)) {
        group = json_string_value(j);
    }
    j = json_object_get(row, "script_name");
    if (j && json_is_string(j)) {
        script = json_string_value(j);
    }
    j = json_object_get(row, "summary");
    if (j && json_is_string(j)) {
        summary = json_string_value(j);
    }
    if (group && script) {
        char name_buf[512];
        snprintf(name_buf, sizeof(name_buf), "%s.%s", group, script);
        lua_pushstring(L, name_buf);
        lua_setfield(L, -2, "name");
    }
    if (group) {
        lua_pushstring(L, group);
        lua_setfield(L, -2, "group");
    }
    if (script) {
        lua_pushstring(L, script);
        lua_setfield(L, -2, "script");
    }
    if (summary) {
        lua_pushstring(L, summary);
        lua_setfield(L, -2, "summary");
    }
    H_lua_mcp_push_decoded_json_field(L, row, "mcp_schema");
    lua_setfield(L, -2, "schema");
    H_lua_mcp_push_decoded_json_field(L, row, "mcp_annotations");
    lua_setfield(L, -2, "annotations");
}

int H_lua_mcp_list(lua_State* L) {
    lua_Integer cursor;
    lua_Integer page_size;
    char* data_json;
    json_error_t err;
    json_t* root;
    size_t total;
    size_t i;
    size_t out_i;
    size_t end;

    cursor = 1;
    page_size = H_MCP_DEFAULT_PAGE_SIZE;
    if (lua_gettop(L) >= 1 && lua_isinteger(L, 1)) {
        cursor = lua_tointeger(L, 1);
    } else if (lua_gettop(L) >= 1 && lua_isstring(L, 1)) {
        cursor = (lua_Integer)atoi(lua_tostring(L, 1));
    }
    if (lua_gettop(L) >= 2 && lua_isinteger(L, 2)) {
        page_size = lua_tointeger(L, 2);
    }
    if (cursor < 1) {
        cursor = 1;
    }
    if (page_size < 1) {
        page_size = H_MCP_DEFAULT_PAGE_SIZE;
    }
    if (page_size > H_MCP_MAX_PAGE_SIZE) {
        page_size = H_MCP_MAX_PAGE_SIZE;
    }

    data_json = H_lua_mcp_fetch_list_rows_json();
    lua_newtable(L);
    if (!data_json) {
        lua_pushnil(L);
        return 2;
    }
    root = json_loads(data_json, 0, &err);
    free(data_json);
    if (!root || !json_is_array(root)) {
        if (root) {
            json_decref(root);
        }
        lua_pushnil(L);
        return 2;
    }
    total = json_array_size(root);
    out_i = 1;
    end = (size_t)cursor + (size_t)page_size - 1;
    for (i = (size_t)cursor; i <= total && i <= end; i++) {
        json_t* row = json_array_get(root, i - 1);
        if (row && json_is_object(row)) {
            H_lua_mcp_push_row(L, row);
            lua_rawseti(L, -2, (lua_Integer)out_i);
            out_i++;
        }
    }
    if (end < total) {
        lua_pushinteger(L, (lua_Integer)end + 1);
    } else {
        lua_pushnil(L);
    }
    json_decref(root);
    return 2;
}

char* H_lua_mcp_parent_hydrogen_json(lua_State* L) {
    char* json;

    lua_getglobal(L, "params");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return NULL;
    }
    lua_getfield(L, -1, "_hydrogen");
    lua_remove(L, -2);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return NULL;
    }
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return NULL;
    }
    json = H_lua_table_to_json_string(L, lua_gettop(L));
    lua_pop(L, 1);
    return json;
}

char* H_lua_mcp_build_tool_params(lua_State* L, int args_idx, char** err_out) {
    json_t* args;
    char* hydrogen_json;
    char* out;
    json_error_t err;

    *err_out = NULL;
    if (lua_istable(L, args_idx)) {
        lua_getfield(L, args_idx, "_hydrogen");
        if (!lua_isnil(L, -1)) {
            lua_pop(L, 1);
            *err_out = strdup("reserved _hydrogen");
            return NULL;
        }
        lua_pop(L, 1);
        args = H_lua_value_to_json(L, args_idx, 0);
        if (!args) {
            args = json_object();
        }
    } else {
        args = json_object();
    }
    if (!args) {
        *err_out = strdup("allocation failed");
        return NULL;
    }
    hydrogen_json = H_lua_mcp_parent_hydrogen_json(L);
    if (hydrogen_json) {
        json_t* hydrogen = json_loads(hydrogen_json, 0, &err);
        free(hydrogen_json);
        if (hydrogen) {
            json_object_set_new(args, "_hydrogen", hydrogen);
        }
    }
    out = json_dumps(args, JSON_COMPACT);
    json_decref(args);
    return out;
}

char* H_lua_mcp_load_tool_source(const char* group, const char* script) {
    const char* database;
    int timeout;

    if (H_lua_mcp_fetch_source_hook) {
        return H_lua_mcp_fetch_source_hook(group, script);
    }
    database = orchestrator_resolve_database();
    if (!database) {
        return NULL;
    }
    timeout = H_MCP_LIST_TIMEOUT_SECONDS;
    if (app_config && app_config->mcp.RequestTimeoutSeconds > 0) {
        timeout = app_config->mcp.RequestTimeoutSeconds;
    }
    return scripting_fetch_mcp_script_source(group, script, database, timeout);
}

int H_lua_mcp_call(lua_State* L) {
    const char* name;
    char* group;
    char* script;
    char* err;
    char* source;
    char* params_json;
    lua_State* child;
    int rc;
    const char* captured;
    json_error_t jerr;
    json_t* captured_json;
    int args_idx;

    if (lua_gettop(L) < 1 || !lua_isstring(L, 1)) {
        lua_pushnil(L);
        lua_pushstring(L, "H.mcp.call: name required");
        return 2;
    }
    name = lua_tostring(L, 1);
    if (!scripting_invoke_parse_script_name(name, &group, &script)) {
        lua_pushnil(L);
        lua_pushstring(L, "not found");
        return 2;
    }
    args_idx = 2;
    params_json = H_lua_mcp_build_tool_params(L, args_idx, &err);
    if (err) {
        free(group);
        free(script);
        lua_pushnil(L);
        lua_pushstring(L, err);
        free(err);
        return 2;
    }
    source = H_lua_mcp_load_tool_source(group, script);
    free(group);
    free(script);
    if (!source) {
        free(params_json);
        lua_pushnil(L);
        lua_pushstring(L, "not found");
        return 2;
    }
    child = H_lua_create_context();
    if (!child) {
        free(source);
        free(params_json);
        lua_pushnil(L);
        lua_pushstring(L, "H.mcp.call: child state failed");
        return 2;
    }
    lua_getglobal(child, "H");
    lua_pushcfunction(child, H_lua_mcp_capture_result_json);
    lua_setfield(child, -2, "set_result_json");
    lua_pop(child, 1);
    H_lua_inject_job_params(child, params_json);
    free(params_json);
    rc = H_lua_run_string(child, source, "[mcp.call]");
    free(source);
    if (rc != LUA_OK) {
        const char* lua_err = lua_tostring(child, -1);
        lua_pushnil(L);
        lua_pushstring(L, lua_err ? lua_err : "tool error");
        H_lua_destroy_context(child);
        return 2;
    }
    lua_pushlightuserdata(child, (void*)&H_lua_mcp_inline_result_key);
    lua_gettable(child, LUA_REGISTRYINDEX);
    captured = lua_isstring(child, -1) ? lua_tostring(child, -1) : NULL;
    if (captured) {
        captured_json = json_loads(captured, 0, &jerr);
        if (captured_json) {
            push_json_value_as_lua(L, captured_json);
            json_decref(captured_json);
        } else {
            lua_newtable(L);
        }
    } else if (lua_istable(child, -2) || lua_istable(child, lua_gettop(child) - 1)) {
        lua_newtable(L);
    } else {
        lua_newtable(L);
    }
    lua_pushnil(L);
    H_lua_destroy_context(child);
    return 2;
}

int H_lua_mcp_call_async(lua_State* L) {
    const char* name;
    char* group;
    char* script;
    char* err;
    char* source;
    char* params_json;
    char* job_id;
    H_Handle* h;

    if (lua_gettop(L) < 1 || !lua_isstring(L, 1)) {
        h = H_Handle_new(L, H_HK_MCP);
        if (!h) {
            lua_pushnil(L);
            return 1;
        }
        h->error = strdup("H.mcp.call_async: name required");
        return 1;
    }
    name = lua_tostring(L, 1);
    if (!scripting_invoke_parse_script_name(name, &group, &script)) {
        h = H_Handle_new(L, H_HK_MCP);
        if (!h) {
            lua_pushnil(L);
            return 1;
        }
        h->error = strdup("not found");
        return 1;
    }
    params_json = H_lua_mcp_build_tool_params(L, 2, &err);
    if (err) {
        free(group);
        free(script);
        h = H_Handle_new(L, H_HK_MCP);
        if (!h) {
            free(err);
            lua_pushnil(L);
            return 1;
        }
        h->error = err;
        return 1;
    }
    source = H_lua_mcp_load_tool_source(group, script);
    free(group);
    free(script);
    if (!source) {
        free(params_json);
        h = H_Handle_new(L, H_HK_MCP);
        if (!h) {
            lua_pushnil(L);
            return 1;
        }
        h->error = strdup("not found");
        return 1;
    }
    H_lua_mcp_submit_count++;
    if (H_lua_mcp_submit_job_hook) {
        job_id = H_lua_mcp_submit_job_hook(name, source, params_json);
    } else {
        job_id = scripting_submit_job_with_source(name, source, params_json);
    }
    free(source);
    free(params_json);
    h = H_Handle_new(L, H_HK_MCP);
    if (!h) {
        free(job_id);
        lua_pushnil(L);
        return 1;
    }
    if (!job_id) {
        h->error = strdup("H.mcp.call_async: submit failed");
        return 1;
    }
    h->query_id = job_id;
    return 1;
}

int H_lua_mcp_wait_one(lua_State* L, H_Handle* h) {
    ScriptingWaitResult wr;
    ScoreboardEntry* entry;
    int timeout;
    json_error_t err;
    json_t* root;

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
    if (H_lua_mcp_wait_job_hook) {
        return H_lua_mcp_wait_job_hook(L, h);
    }
    if (!h->query_id) {
        lua_pushnil(L);
        lua_pushstring(L, "H.wait: handle has no pending job");
        h->consumed = true;
        return 2;
    }
    timeout = H_MCP_LIST_TIMEOUT_SECONDS;
    if (app_config && app_config->mcp.RequestTimeoutSeconds > 0) {
        timeout = app_config->mcp.RequestTimeoutSeconds;
    }
    entry = NULL;
    wr = scripting_wait_job(h->query_id, timeout, &entry);
    h->consumed = true;
    if (wr != SCRIPTING_WAIT_COMPLETED) {
        if (entry) {
            scoreboard_entry_free(entry);
        }
        lua_pushnil(L);
        lua_pushstring(L, scripting_wait_result_name(wr));
        return 2;
    }
    if (entry && entry->result_json) {
        root = json_loads(entry->result_json, 0, &err);
        if (root) {
            push_json_value_as_lua(L, root);
            json_decref(root);
        } else {
            lua_newtable(L);
        }
    } else {
        lua_newtable(L);
    }
    lua_pushnil(L);
    if (entry) {
        scoreboard_entry_free(entry);
    }
    return 2;
}

void H_lua_install_mcp(lua_State* L) {
    if (!L) {
        return;
    }
    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_SCRIPTING, "H_lua_install_mcp: H table missing",
                 LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }
    lua_newtable(L);
    lua_pushcfunction(L, H_lua_mcp_list);
    lua_setfield(L, -2, "list");
    lua_pushcfunction(L, H_lua_mcp_call);
    lua_setfield(L, -2, "call");
    lua_pushcfunction(L, H_lua_mcp_call_async);
    lua_setfield(L, -2, "call_async");
    lua_setfield(L, -2, "mcp");
    lua_pop(L, 1);
}
