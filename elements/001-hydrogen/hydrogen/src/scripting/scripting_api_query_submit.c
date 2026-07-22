/*
 * Scripting Subsystem - Host API: H.query, H.altquery, H.authquery
 *
 * Phase 13 of the LUA_PLAN: conduit-equivalent async query functions.
 */

#include <src/hydrogen.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <jansson.h>

#include "scripting_api.h"
#include "scripting_api_internal.h"
#include "scripting_handle.h"
#include "lua_context.h"

#include <src/api/conduit/conduit_helpers.h>
#include <src/api/auth/auth_service.h>

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

int get_default_query_timeout(void);

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