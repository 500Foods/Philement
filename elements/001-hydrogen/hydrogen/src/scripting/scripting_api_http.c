/*
 * Scripting Subsystem - Host API: H.http (HTTP Client)
 *
 * Phase 16/17 of the LUA_PLAN. H.http.get / H.http.post return an
 * opaque H_Handle; H.wait resolves it. Sync wrappers also provided.
 */

// Project includes
#include <src/hydrogen.h>

// System includes
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Third-party includes
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <jansson.h>
#include <curl/curl.h>

// Local includes
#include "scripting_api.h"
#include "scripting_api_internal.h"
#include "scripting_handle.h"
#include "http_client.h"
#include "http_pool.h"
#include "src/api/auth/oidc_rp/oidc_rp_http.h"

struct curl_slist* H_lua_headers_to_slist(lua_State* L, int idx) {
    if (!lua_istable(L, idx)) {
        return NULL;
    }
    struct curl_slist* list = NULL;

    lua_pushnil(L);
    while (lua_next(L, idx) != 0) {
        const char* name = NULL;
        const char* value = NULL;
        size_t name_len = 0;
        size_t value_len = 0;

        if (lua_isstring(L, -2)) {
            name = lua_tolstring(L, -2, &name_len);
        }
        if (lua_isstring(L, -1)) {
            value = lua_tolstring(L, -1, &value_len);
        }
        if (name && value && name_len > 0) {
            size_t buf_len = name_len + value_len + 4;
            if (buf_len > 8192) {
                log_this(SR_LUA,
                         "H_lua_headers_to_slist: header '%s' too long (%zu bytes), skipping",
                         LOG_LEVEL_ALERT, 2, name, name_len + value_len);
            } else {
                char* header = malloc(buf_len);
                if (header) {
                    int n = snprintf(header, buf_len, "%s: %s", name, value);
                    if (n > 0 && (size_t)n < buf_len) {
                        list = curl_slist_append(list, header);
                    }
                    free(header);
                }
            }
        } else {
            log_this(SR_LUA,
                     "H_lua_headers_to_slist: skipping non-string header (key or value)",
                     LOG_LEVEL_ALERT, 0);
        }
        lua_pop(L, 1);
    }
    return list;
}

int H_lua_opts_timeout(lua_State* L, int idx) {
    if (!lua_istable(L, idx)) {
        return -1;
    }
    lua_getfield(L, idx, "timeout");
    int result = -1;
    if (lua_isnumber(L, -1)) {
        double v = lua_tonumber(L, -1);
        if (v > 0 && v <= (double)INT_MAX) {
            result = (int)v;
        }
    }
    lua_pop(L, 1);
    return result;
}

int H_lua_http_get(lua_State* L) {
    int nargs = lua_gettop(L);
    if (nargs < 1) {
        H_Handle* h = H_Handle_new(L, H_HK_HTTP);
        if (h) h->error = strdup("H.http.get: missing url argument");
        return 1;
    }
    const char* url = NULL;
    if (lua_isstring(L, 1)) {
        url = lua_tostring(L, 1);
    }
    if (!url || !*url) {
        H_Handle* h = H_Handle_new(L, H_HK_HTTP);
        if (h) h->error = strdup("H.http.get: url is not a non-empty string");
        return 1;
    }

    struct curl_slist* headers = NULL;
    if (nargs >= 2 && !lua_isnil(L, 2)) {
        headers = H_lua_headers_to_slist(L, 2);
    }

    int timeout = (nargs >= 3) ? H_lua_opts_timeout(L, 3) : -1;

    H_Handle* h = H_Handle_new(L, H_HK_HTTP);
    if (!h) {
        if (headers) curl_slist_free_all(headers);
        return 0;
    }
    h->http_url = strdup(url);
    h->http_method = strdup("GET");
    h->http_timeout = timeout;
    h->http_headers_slist = headers;
    if (!h->http_url || !h->http_method) {
        h->error = strdup("H.http.get: allocation failed");
        return 1;
    }
    return 1;
}

int H_lua_http_post(lua_State* L) {
    int nargs = lua_gettop(L);
    if (nargs < 1) {
        H_Handle* h = H_Handle_new(L, H_HK_HTTP);
        if (h) h->error = strdup("H.http.post: missing url argument");
        return 1;
    }
    const char* url = NULL;
    if (lua_isstring(L, 1)) {
        url = lua_tostring(L, 1);
    }
    if (!url || !*url) {
        H_Handle* h = H_Handle_new(L, H_HK_HTTP);
        if (h) h->error = strdup("H.http.post: url is not a non-empty string");
        return 1;
    }

    const char* body = NULL;
    int arg_idx = 2;
    if (nargs >= 2 && lua_isstring(L, 2)) {
        body = lua_tostring(L, 2);
        arg_idx = 3;
    } else if (nargs >= 2 && !lua_isnil(L, 2) && !lua_istable(L, 2)) {
        H_Handle* h = H_Handle_new(L, H_HK_HTTP);
        if (h) h->error = strdup("H.http.post: body must be a string or nil");
        return 1;
    }

    struct curl_slist* headers = NULL;
    if (nargs >= arg_idx && lua_istable(L, arg_idx)) {
        headers = H_lua_headers_to_slist(L, arg_idx);
        arg_idx++;
    } else if (nargs >= arg_idx && !lua_isnil(L, arg_idx)) {
        H_Handle* h = H_Handle_new(L, H_HK_HTTP);
        if (h) h->error = strdup("H.http.post: headers must be a table or nil");
        return 1;
    } else if (nargs >= arg_idx) {
        arg_idx++;
    }

    int timeout = -1;
    const char* content_type = NULL;
    if (nargs >= arg_idx && lua_istable(L, arg_idx)) {
        timeout = H_lua_opts_timeout(L, arg_idx);
        lua_getfield(L, arg_idx, "content_type");
        if (lua_isstring(L, -1)) {
            content_type = lua_tostring(L, -1);
        }
        lua_pop(L, 1);
    }

    H_Handle* h = H_Handle_new(L, H_HK_HTTP);
    if (!h) {
        if (headers) curl_slist_free_all(headers);
        return 0;
    }
    h->http_url = strdup(url);
    h->http_method = strdup("POST");
    if (body) h->http_body = strdup(body);
    if (content_type) h->http_content_type = strdup(content_type);
    h->http_timeout = timeout;
    h->http_headers_slist = headers;
    if (!h->http_url || !h->http_method) {
        h->error = strdup("H.http.post: allocation failed");
        return 1;
    }
    return 1;
}

/*
 * Perform the HTTP call for one handle and push result/error pair.
 */
int H_lua_http_wait_one(lua_State* L, H_Handle* h) {
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
    if (!h->http_url || !h->http_method) {
        lua_pushnil(L);
        lua_pushstring(L, "H.wait: HTTP handle missing url or method");
        h->consumed = true;
        return 2;
    }

    // Phase 17: submit to HTTP worker pool. Fallback to inline call
    // when pool is not running.
    if (!scripting_http_pool_submit(h)) {
        struct curl_slist* headers = (struct curl_slist*)h->http_headers_slist;
        h->http_headers_slist = NULL;

        struct timespec start_ts;
        clock_gettime(CLOCK_MONOTONIC, &start_ts);

        struct OidcRpHttpResponse* resp;
        if (strcmp(h->http_method, "GET") == 0) {
            resp = scripting_http_get(h->http_url, headers,
                                      h->http_timeout, true);
        } else if (strcmp(h->http_method, "POST") == 0) {
            resp = scripting_http_post(h->http_url, h->http_body,
                                       h->http_content_type, headers,
                                       h->http_timeout, true);
        } else {
            if (headers) curl_slist_free_all(headers);
            lua_pushnil(L);
            lua_pushstring(L, "H.wait: unknown HTTP method on handle");
            h->consumed = true;
            return 2;
        }

        h->consumed = true;

        if (!resp) {
            lua_pushnil(L);
            lua_pushstring(L, "H.wait: HTTP call returned no response");
            return 2;
        }

        struct timespec end_ts;
        clock_gettime(CLOCK_MONOTONIC, &end_ts);
        long elapsed_ms = (long)(((end_ts.tv_sec - start_ts.tv_sec) * 1000L) +
                                 ((end_ts.tv_nsec - start_ts.tv_nsec) / 1000000L));

        return H_lua_http_push_inline_result(L, resp, elapsed_ms);
    }

    // Pool path: block on the handle's condvar.
    pthread_mutex_lock(&h->http_mutex);
    while (!h->http_ready) {
        pthread_cond_wait(&h->http_cond, &h->http_mutex);
    }

    pthread_mutex_unlock(&h->http_mutex);

    return H_lua_http_push_pool_result(L, h);
}

/*
 * Build the (result, error) pair for a completed pool-submitted HTTP
 * handle and push it onto L, reading the fields the worker stored on the
 * handle (http_result_status / _body / _headers_json / _error /
 * _elapsed_ms).
 *
 * On error (http_result_error set) pushes (nil, "H.wait: <err>") and
 * returns. On success pushes a result table
 * { status, headers, body, elapsed_ms } followed by a trailing nil (so
 * the caller's (result, err) pair contract holds).
 *
 * Extracted from H_lua_http_wait_one so the pool-result marshalling can
 * be unit tested directly with a handle whose result fields are
 * populated, without standing up the worker pool.
 *
 * Returns 2 (the size of the pushed pair).
 */
int H_lua_http_push_pool_result(lua_State* L, H_Handle* h) {
    h->consumed = true;

    long elapsed_ms = h->http_result_elapsed_ms;
    int status = h->http_result_status;
    char* body_copy = h->http_result_body ? strdup(h->http_result_body) : NULL;
    char* headers_json_copy = h->http_result_headers_json
        ? strdup(h->http_result_headers_json) : NULL;
    char* error_copy = h->http_result_error ? strdup(h->http_result_error) : NULL;

    if (error_copy) {
        char buf[512];
        snprintf(buf, sizeof(buf), "H.wait: %s", error_copy);
        lua_pushnil(L);
        lua_pushstring(L, buf);
        free(error_copy);
        free(body_copy);
        free(headers_json_copy);
        return 2;
    }

    lua_createtable(L, 0, 4);
    lua_pushinteger(L, (lua_Integer)status);
    lua_setfield(L, -2, "status");

    lua_createtable(L, 0, 0);
    if (headers_json_copy) {
        json_error_t jerr;
        json_t* arr = json_loads(headers_json_copy, 0, &jerr);
        if (arr && json_is_array(arr)) {
            size_t i;
            json_t* obj;
            json_array_foreach(arr, i, obj) {
                const char* name = NULL;
                const char* value = NULL;
                json_t* jname = json_object_get(obj, "name");
                json_t* jval = json_object_get(obj, "value");
                if (json_is_string(jname)) name = json_string_value(jname);
                if (json_is_string(jval))  value = json_string_value(jval);
                if (!name) name = "";
                if (!value) value = "";
                lua_getfield(L, -1, name);
                if (lua_isnil(L, -1)) {
                    lua_pop(L, 1);
                    lua_pushstring(L, value);
                    lua_setfield(L, -2, name);
                } else {
                    size_t existing_len = 0;
                    const char* existing = lua_tolstring(L, -1, &existing_len);
                    size_t total = existing_len + strlen(value) + 3;
                    char* combined = malloc(total);
                    if (combined) {
                        snprintf(combined, total, "%s, %s", existing, value);
                        lua_pop(L, 1);
                        lua_pushstring(L, combined);
                        lua_setfield(L, -2, name);
                        free(combined);
                    } else {
                        lua_pop(L, 1);
                    }
                }
            }
        }
        if (arr) json_decref(arr);
    }
    lua_setfield(L, -2, "headers");

    if (body_copy) {
        lua_pushstring(L, body_copy);
    } else {
        lua_pushliteral(L, "");
    }
    lua_setfield(L, -2, "body");

    lua_pushinteger(L, (lua_Integer)elapsed_ms);
    lua_setfield(L, -2, "elapsed_ms");

    lua_pushnil(L);

    free(body_copy);
    free(headers_json_copy);
    return 2;
}

/*
 * Build the (result, error) pair for a completed inline HTTP response
 * and push it onto L. elapsed_ms is the caller-measured call duration.
 *
 * On error (resp->error_message set) pushes (nil, error_string) and
 * frees resp. On success pushes a result table
 * { status, headers, body, elapsed_ms } followed by a trailing nil
 * (so the caller's (result, err) pair contract holds) and frees resp.
 *
 * Extracted from H_lua_http_wait_one so the response-to-table logic can
 * be unit tested directly with synthetic OidcRpHttpResponse objects.
 *
 * Returns 2 (the size of the pushed pair).
 */
int H_lua_http_push_inline_result(lua_State* L, OidcRpHttpResponse* resp, long elapsed_ms) {
    if (resp->error_message) {
        char buf[512];
        snprintf(buf, sizeof(buf), "H.wait: %s", resp->error_message);
        lua_pushnil(L);
        lua_pushstring(L, buf);
        oidc_rp_http_response_free(resp);
        return 2;
    }

    lua_createtable(L, 0, 4);
    lua_pushinteger(L, (lua_Integer)resp->http_status);
    lua_setfield(L, -2, "status");

    lua_createtable(L, 0, (int)resp->headers_count);
    for (size_t i = 0; i < resp->headers_count; i++) {
        const char* name = resp->headers[i].name ? resp->headers[i].name : "";
        const char* value = resp->headers[i].value ? resp->headers[i].value : "";
        lua_getfield(L, -1, name);
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1);
            lua_pushstring(L, value);
            lua_setfield(L, -2, name);
        } else {
            size_t existing_len = 0;
            const char* existing = lua_tolstring(L, -1, &existing_len);
            size_t total = existing_len + strlen(value) + 3;
            char* combined = malloc(total);
            if (combined) {
                snprintf(combined, total, "%s, %s", existing, value);
                lua_pop(L, 1);
                lua_pushstring(L, combined);
                lua_setfield(L, -2, name);
                free(combined);
            } else {
                lua_pop(L, 1);
            }
        }
    }
    lua_setfield(L, -2, "headers");

    if (resp->body) {
        lua_pushstring(L, resp->body);
    } else {
        lua_pushliteral(L, "");
    }
    lua_setfield(L, -2, "body");

    lua_pushinteger(L, (lua_Integer)elapsed_ms);
    lua_setfield(L, -2, "elapsed_ms");

    lua_pushnil(L);

    oidc_rp_http_response_free(resp);
    return 2;
}

int H_lua_http_get_sync(lua_State* L) {
    int n_pushed = H_lua_http_get(L);
    if (n_pushed == 0) {
        lua_pushnil(L);
        lua_pushstring(L, "H.http.get_sync: handle allocation failed");
        return 2;
    }
    H_Handle* h = H_Handle_check(L, -1);
    if (!h) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, "H.http.get_sync: handle creation failed");
        return 2;
    }
    int n = H_lua_http_wait_one(L, h);
    lua_remove(L, -(n + 1));
    return n;
}

int H_lua_http_post_sync(lua_State* L) {
    int n_pushed = H_lua_http_post(L);
    if (n_pushed == 0) {
        lua_pushnil(L);
        lua_pushstring(L, "H.http.post_sync: handle allocation failed");
        return 2;
    }
    H_Handle* h = H_Handle_check(L, -1);
    if (!h) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, "H.http.post_sync: handle creation failed");
        return 2;
    }
    int n = H_lua_http_wait_one(L, h);
    lua_remove(L, -(n + 1));
    return n;
}

void H_lua_install_http(lua_State* L) {
    if (!L) return;

    H_Handle_install_metatable(L);

    lua_getglobal(L, "H");
    if (!lua_istable(L, -1)) {
        log_this(SR_LUA, "H_lua_install_http: H table missing",
                 LOG_LEVEL_ERROR, 0);
        lua_pop(L, 1);
        return;
    }

    lua_newtable(L);
    lua_pushcfunction(L, H_lua_http_get);        lua_setfield(L, -2, "get");
    lua_pushcfunction(L, H_lua_http_post);       lua_setfield(L, -2, "post");
    lua_pushcfunction(L, H_lua_http_get_sync);   lua_setfield(L, -2, "get_sync");
    lua_pushcfunction(L, H_lua_http_post_sync);  lua_setfield(L, -2, "post_sync");
    lua_setfield(L, -2, "http");

    lua_pop(L, 1);
}
