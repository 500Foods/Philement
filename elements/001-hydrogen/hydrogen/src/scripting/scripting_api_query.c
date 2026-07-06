/*
 * Scripting Subsystem - Host API: H.query, H.altquery, H.authquery,
 * H.wait, and sync wrappers
 *
 * Phase 13 of the LUA_PLAN: conduit-equivalent async query functions.
 */

// Project includes
#include <src/hydrogen.h>

// System includes
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// Third-party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <jansson.h>

// Local includes
#include "scripting_api.h"
#include "scripting_api_internal.h"
#include "scripting_handle.h"
#include "lua_context.h"

// Phase 13 includes
#include <src/api/conduit/conduit_helpers.h>
#include <src/api/auth/auth_service.h>

// Phase 13 implementation //////////////////////////////////////////////////

DatabaseQueue* resolve_db_queue(const char* db_name,
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

int H_lua_submit_query(lua_State* L,
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
        .queue_type_hint = 0,
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

    H_Handle* h = H_Handle_new(L, H_HK_QUERY);
    if (!h) {
        return 0;
    }
    h->query_id = query_id;
    h->db_queue = dbq;
    return 1;
}

int get_default_query_timeout(void) {
    if (app_config && app_config->scripting.DefaultQueryTimeout > 0) {
        return app_config->scripting.DefaultQueryTimeout;
    }
    return 30;
}

/*
 * H.query(sql, params, opts?) -> handle
 */
int H_lua_query(lua_State* L) {
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
        return 0;
    }
    return pushed;
}

/*
 * H.altquery(database_name, sql, params, opts?) -> handle
 */
int H_lua_altquery(lua_State* L) {
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

char* validate_jwt_and_get_db(const char* token, char** err_out) {
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
 */
int H_lua_authquery(lua_State* L) {
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
 * Wait on a single handle and push the result/error pair.
 */
int H_lua_wait_one(lua_State* L, H_Handle* h) {
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
    if (h->kind == H_HK_LLM) {
        return H_lua_llm_wait_one(L, h);
    }
    if (!h->query_id || !h->db_queue) {
        lua_pushnil(L);
        lua_pushstring(L, "H.wait: handle has no pending query");
        h->consumed = true;
        return 2;
    }

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

    if (result_q->error_message) {
        lua_pushnil(L);
        lua_pushstring(L, result_q->error_message);
    } else {
        H_lua_build_result_table(L, result_q->query_template, result_q->affected_rows);
        lua_pushnil(L);
    }

    free(result_q->query_id);
    free(result_q->query_template);
    free(result_q->error_message);
    free(result_q);

    return 2;
}

/*
 * H.wait(handle1, handle2, ...) -> r1, r2, ..., rN, e1, e2, ..., eN
 */
int H_lua_wait(lua_State* L) {
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
        if (h->kind == H_HK_LLM) {
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
            errors[i] = NULL;
        }
        free(result_q->query_id);
        free(result_q->query_template);
        free(result_q->error_message);
        free(result_q);
    }

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
 * Sync wrappers: submit + wait in one call.
 */
int H_lua_query_sync(lua_State* L) {
    int n_pushed = H_lua_query(L);
    if (n_pushed == 0) {
        lua_pushnil(L);
        lua_pushstring(L, "H.query_sync: handle allocation failed");
        return 2;
    }
    H_Handle* h = H_Handle_check(L, -1);
    if (!h) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, "H.query_sync: handle creation failed");
        return 2;
    }
    int n = H_lua_wait_one(L, h);
    lua_remove(L, -(n + 1));
    return n;
}

int H_lua_altquery_sync(lua_State* L) {
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

int H_lua_authquery_sync(lua_State* L) {
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

// Install functions ////////////////////////////////////////////////////////

void H_lua_install_query(lua_State* L) {
    if (!L) return;

    H_Handle_install_metatable(L);

    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_query: H table missing",
                 LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }

    lua_pushcfunction(L, H_lua_query);        lua_setfield(L, -2, "query");
    lua_pushcfunction(L, H_lua_altquery);     lua_setfield(L, -2, "altquery");
    lua_pushcfunction(L, H_lua_authquery);    lua_setfield(L, -2, "authquery");
    lua_pushcfunction(L, H_lua_wait);         lua_setfield(L, -2, "wait");

    lua_pushcfunction(L, H_lua_query_sync);        lua_setfield(L, -2, "query_sync");
    lua_pushcfunction(L, H_lua_altquery_sync);     lua_setfield(L, -2, "altquery_sync");
    lua_pushcfunction(L, H_lua_authquery_sync);    lua_setfield(L, -2, "authquery_sync");

    H_lua_install_http(L);

    lua_pop(L, 1);
}
