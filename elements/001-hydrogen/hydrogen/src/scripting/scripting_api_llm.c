/*
 * Scripting Subsystem - Host API: H.llm (LLM Calls)
 *
 * Phase 18 of the LUA_PLAN. Provides H.llm.call and H.llm.list
 * for invoking language models from Lua scripts.
 */

#include <src/hydrogen.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <jansson.h>

#include "scripting_api.h"
#include "scripting_api_internal.h"
#include "scripting_handle.h"
#include "lua_context.h"
#include "http_client.h"
#include "scripting_api_llm.h"

#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/proxy.h>

extern ServiceThreads scripting_threads;
extern volatile sig_atomic_t scripting_system_shutdown;

#define DEFAULT_LLM_TIMEOUT 300
#define DEFAULT_LLM_MAX_TOKENS 4096
#define DEFAULT_LLM_TEMPERATURE 0.7

DatabaseQueue* resolve_llm_db_queue(const char* db_name, char** err_out) {
    if (err_out) *err_out = NULL;

    const char* target = db_name;
    if (!target || !*target) {
        if (!app_config) {
            if (err_out) {
                *err_out = strdup("H.llm.call: no app_config available");
            }
            return NULL;
        }
        target = app_config->scripting.DefaultDatabase;
        if (!target || !*target) {
            if (app_config->databases.connection_count == 1 &&
                app_config->databases.connections[0].name &&
                app_config->databases.connections[0].name[0] != '\0') {
                target = app_config->databases.connections[0].name;
            } else {
                if (err_out) {
                    *err_out = strdup("H.llm.call: no database specified and no "
                                    "scripting.DefaultDatabase configured");
                }
                return NULL;
            }
        }
    }

    if (!global_queue_manager) {
        if (err_out) {
            *err_out = strdup("H.llm.call: database queue manager not available");
        }
        return NULL;
    }

    DatabaseQueue* dbq = database_queue_manager_get_database(
        global_queue_manager, target);
    if (!dbq) {
        if (err_out) {
            char buf[256];
            snprintf(buf, sizeof(buf),
                     "H.llm.call: database '%s' not found", target);
            *err_out = strdup(buf);
        }
        return NULL;
    }
    return dbq;
}

ChatEngineConfig* resolve_llm_engine(const char* model_name, const char* db_name) {
    if (!model_name || !*model_name) {
        return NULL;
    }

    DatabaseQueue* dbq = resolve_llm_db_queue(db_name, NULL);
    if (!dbq || !dbq->chat_engine_cache) {
        return NULL;
    }

    return chat_engine_cache_lookup_by_name(dbq->chat_engine_cache, model_name);
}

char* build_llm_request_json(const char* prompt, int max_tokens, double temperature) {
    if (!prompt) {
        return NULL;
    }

    json_t* root = json_object();
    if (!root) {
        return NULL;
    }

    json_object_set_new(root, "prompt", json_string(prompt));

    if (max_tokens > 0) {
        json_object_set_new(root, "max_tokens", json_integer(max_tokens));
    }

    if (temperature > 0.0 && temperature < 2.0) {
        json_object_set_new(root, "temperature", json_real(temperature));
    }

    char* json_str = json_dumps(root, JSON_COMPACT);
    json_decref(root);
    return json_str;
}

int H_lua_llm_call(lua_State* L) {
    int nargs = lua_gettop(L);
    if (nargs < 1) {
        H_Handle* h = H_Handle_new(L, H_HK_LLM);
        if (h) h->llm_error = strdup("H.llm.call: missing model argument");
        return 1;
    }

    const char* model_name = NULL;
    if (lua_isstring(L, 1)) {
        model_name = lua_tostring(L, 1);
    }
    if (!model_name || !*model_name) {
        H_Handle* h = H_Handle_new(L, H_HK_LLM);
        if (h) h->llm_error = strdup("H.llm.call: model is not a non-empty string");
        return 1;
    }

    const char* prompt = NULL;
    int prompt_idx = 2;
    if (nargs >= 2 && !lua_isnil(L, 2)) {
        if (lua_isstring(L, 2)) {
            prompt = lua_tostring(L, 2);
            prompt_idx = 3;
        } else if (!lua_istable(L, 2)) {
            H_Handle* h = H_Handle_new(L, H_HK_LLM);
            if (h) h->llm_error = strdup("H.llm.call: prompt must be a string or table");
            return 1;
        }
    }

    int max_tokens = 0;
    double temperature = 0.0;
    const char* db_name = NULL;
    int timeout = DEFAULT_LLM_TIMEOUT;

    if (nargs >= prompt_idx && lua_istable(L, prompt_idx)) {
        lua_getfield(L, prompt_idx, "max_tokens");
        if (lua_isnumber(L, -1)) {
            max_tokens = (int)lua_tonumber(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, prompt_idx, "temperature");
        if (lua_isnumber(L, -1)) {
            temperature = lua_tonumber(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, prompt_idx, "database");
        if (lua_isstring(L, -1)) {
            db_name = lua_tostring(L, -1);
        }
        lua_pop(L, 1);

        lua_getfield(L, prompt_idx, "timeout");
        if (lua_isnumber(L, -1)) {
            int t = (int)lua_tonumber(L, -1);
            if (t > 0) timeout = t;
        }
        lua_pop(L, 1);
    }

    H_Handle* h = H_Handle_new(L, H_HK_LLM);
    if (!h) {
        return 0;
    }

    h->llm_model_name = strdup(model_name);
    if (prompt) {
        h->llm_prompt = strdup(prompt);
    }
    h->llm_max_tokens = max_tokens;
    h->llm_temperature = temperature;
    if (db_name) {
        h->llm_db_name = strdup(db_name);
    }
    h->llm_timeout = timeout;

    if (!h->llm_model_name) {
        h->llm_error = strdup("H.llm.call: allocation failed");
        return 1;
    }

    return 1;
}

int H_lua_llm_list(lua_State* L) {
    H_Handle* h = H_Handle_new(L, H_HK_LLM);
    if (!h) {
        return 0;
    }

    h->llm_list = true;

    if (app_config && app_config->scripting.DefaultDatabase) {
        h->llm_db_name = strdup(app_config->scripting.DefaultDatabase);
    } else if (app_config && app_config->databases.connection_count == 1 &&
               app_config->databases.connections[0].name &&
               app_config->databases.connections[0].name[0] != '\0') {
        h->llm_db_name = strdup(app_config->databases.connections[0].name);
    }

    if (!h->llm_db_name) {
        h->llm_error = strdup("H.llm.list: no database available for model enumeration");
        return 1;
    }

    return 1;
}

int H_lua_llm_wait_one(lua_State* L, H_Handle* h) {
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
    if (h->kind != H_HK_LLM) {
        lua_pushnil(L);
        lua_pushstring(L, "H.wait: handle is not an LLM handle");
        return 2;
    }

    if (h->llm_error) {
        lua_pushnil(L);
        lua_pushstring(L, h->llm_error);
        h->consumed = true;
        return 2;
    }

    if (!h->llm_model_name) {
        lua_pushnil(L);
        lua_pushstring(L, "H.wait: LLM handle missing model name");
        h->consumed = true;
        return 2;
    }

    const char* db_name = h->llm_db_name ? h->llm_db_name : NULL;
    ChatEngineConfig* engine = resolve_llm_engine(h->llm_model_name, db_name);

    if (!engine) {
        char buf[256];
        snprintf(buf, sizeof(buf), "H.wait: model '%s' not found", h->llm_model_name);
        lua_pushnil(L);
        lua_pushstring(L, buf);
        h->consumed = true;
        return 2;
    }

    ChatProxyConfig config = chat_proxy_get_default_config();
    config.request_timeout_seconds = h->llm_timeout > 0 ? h->llm_timeout : DEFAULT_LLM_TIMEOUT;

    if (h->llm_list) {
        DatabaseQueue* dbq = resolve_llm_db_queue(db_name, NULL);
        if (!dbq || !dbq->chat_engine_cache) {
            lua_pushnil(L);
            lua_pushstring(L, "H.wait: no chat engine cache available");
            h->consumed = true;
            return 2;
        }

        size_t count = 0;
        ChatEngineConfig** engines = chat_engine_cache_get_all(dbq->chat_engine_cache, &count);

        json_t* arr = json_array();
        if (arr && engines) {
            for (size_t i = 0; i < count; i++) {
                if (engines[i]) {
                    json_t* entry = json_object();
                    json_object_set_new(entry, "name", json_string(engines[i]->name));
                    json_object_set_new(entry, "model", json_string(engines[i]->model));
                    json_object_set_new(entry, "provider", json_string(
                        chat_engine_provider_to_string(engines[i]->provider)));
                    json_object_set_new(entry, "is_default", json_boolean(engines[i]->is_default));
                    json_array_append_new(arr, entry);
                }
            }
            free(engines);

            char* json_str = json_dumps(arr, JSON_COMPACT);
            json_decref(arr);

            lua_createtable(L, 1, 0);
            lua_pushstring(L, json_str);
            lua_setfield(L, -2, "models");
            free(json_str);
        } else {
            lua_pushnil(L);
            lua_pushstring(L, "H.wait: failed to build models list");
            if (arr) json_decref(arr);
            free(engines);
            h->consumed = true;
            return 2;
        }
    } else {
        char* request_json = build_llm_request_json(h->llm_prompt, h->llm_max_tokens, h->llm_temperature);
        if (!request_json) {
            lua_pushnil(L);
            lua_pushstring(L, "H.wait: failed to build request JSON");
            h->consumed = true;
            return 2;
        }

        ChatProxyResult* result = chat_proxy_send_with_retry(engine, request_json, &config);
        free(request_json);

        if (!result) {
            lua_pushnil(L);
            lua_pushstring(L, "H.wait: failed to send LLM request");
            h->consumed = true;
            return 2;
        }

        if (!chat_proxy_result_is_success(result)) {
            char buf[512];
            snprintf(buf, sizeof(buf), "H.wait: LLM request failed (%d): %s",
                     result->http_status,
                     result->error_message ? result->error_message : "unknown error");
            lua_pushnil(L);
            lua_pushstring(L, buf);
            chat_proxy_result_destroy(result);
            h->consumed = true;
            return 2;
        }

        lua_createtable(L, 0, 4);
        lua_pushinteger(L, (lua_Integer)result->http_status);
        lua_setfield(L, -2, "status");

        if (result->response_body) {
            lua_pushstring(L, result->response_body);
        } else {
            lua_pushliteral(L, "");
        }
        lua_setfield(L, -2, "response");

        lua_pushinteger(L, (lua_Integer)(result->total_time_ms));
        lua_setfield(L, -2, "elapsed_ms");

        lua_pushnil(L);

        chat_proxy_result_destroy(result);
    }

    h->consumed = true;
    return 2;
}

int H_lua_llm_call_sync(lua_State* L) {
    int n_pushed = H_lua_llm_call(L);
    if (n_pushed == 0) {
        lua_pushnil(L);
        lua_pushstring(L, "H.llm.call_sync: handle allocation failed");
        return 2;
    }
    H_Handle* h = H_Handle_check(L, -1);
    if (!h) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, "H.llm.call_sync: handle creation failed");
        return 2;
    }
    int n = H_lua_llm_wait_one(L, h);
    lua_remove(L, -(n + 1));
    return n;
}

int H_lua_llm_list_sync(lua_State* L) {
    int n_pushed = H_lua_llm_list(L);
    if (n_pushed == 0) {
        lua_pushnil(L);
        lua_pushstring(L, "H.llm.list_sync: handle allocation failed");
        return 2;
    }
    H_Handle* h = H_Handle_check(L, -1);
    if (!h) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, "H.llm.list_sync: handle creation failed");
        return 2;
    }
    int n = H_lua_llm_wait_one(L, h);
    lua_remove(L, -(n + 1));
    return n;
}

void H_lua_install_llm(lua_State* L) {
    if (!L) return;

    H_Handle_install_metatable(L);

    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_llm: H table missing",
                 LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }

    lua_newtable(L);
    lua_pushcfunction(L, H_lua_llm_call);      lua_setfield(L, -2, "call");
    lua_pushcfunction(L, H_lua_llm_list);      lua_setfield(L, -2, "list");
    lua_pushcfunction(L, H_lua_llm_call_sync); lua_setfield(L, -2, "call_sync");
    lua_pushcfunction(L, H_lua_llm_list_sync); lua_setfield(L, -2, "list_sync");
    lua_setfield(L, -2, "llm");

    lua_pop(L, 1);
}