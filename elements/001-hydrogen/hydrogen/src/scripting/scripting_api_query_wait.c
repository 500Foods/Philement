/*
 * Scripting Subsystem - Wait Functions and Sync Wrappers
 *
 * Phase 13 of the LUA_PLAN: H.wait and synchronous wrappers.
 */

#include <src/hydrogen.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "scripting_api.h"
#include "scripting_api_internal.h"
#include "scripting_handle.h"
#include "lua_context.h"

#include <src/api/conduit/conduit_helpers.h>
#include <src/api/auth/auth_service.h>

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
    if (h->kind == H_HK_MAIL || h->kind == H_HK_NOTIFY) {
        return H_lua_mail_notify_wait_one(L, h);
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
        if (h->kind == H_HK_MAIL || h->kind == H_HK_NOTIFY) {
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

int H_lua_finish_sync_wait(lua_State* L, int n_pushed,
                           const char* alloc_err, const char* create_err) {
    if (n_pushed == 0) {
        lua_pushnil(L);
        lua_pushstring(L, alloc_err);
        return 2;
    }
    H_Handle* h = H_Handle_check(L, -1);
    if (!h) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, create_err);
        return 2;
    }
    int n = H_lua_wait_one(L, h);
    lua_remove(L, -(n + 1));
    return n;
}

int H_lua_query_sync(lua_State* L) {
    return H_lua_finish_sync_wait(L, H_lua_query(L),
                                  "H.query_sync: handle allocation failed",
                                  "H.query_sync: handle creation failed");
}

int H_lua_altquery_sync(lua_State* L) {
    return H_lua_finish_sync_wait(L, H_lua_altquery(L),
                                  "H.altquery_sync: handle allocation failed",
                                  "H.altquery_sync: handle creation failed");
}

int H_lua_authquery_sync(lua_State* L) {
    return H_lua_finish_sync_wait(L, H_lua_authquery(L),
                                  "H.authquery_sync: handle allocation failed",
                                  "H.authquery_sync: handle creation failed");
}