/*
 * Scripting Subsystem - H_Handle (Async Result Handle) Implementation
 *
 * Phase 13 of the LUA_PLAN. See scripting_handle.h for design.
 *
 * The H.Handle metatable is installed once per lua_State and shared
 * across all handles. The metatable has a __gc entry that calls
 * H_Handle_free, so any handle not explicitly consumed by H.wait is
 * freed when Lua's garbage collector collects its userdata.
 *
 * UAF discipline: the handle's fields (query_id, error) are C-owned
 * (strdup'd). The handle itself is C-allocated. The Lua side never
 * reads or writes the handle's fields directly; it only stores an
 * opaque pointer that it passes back to H_Handle_check.
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
#include <curl/curl.h>   // curl_slist_free_all (Phase 16)

// Local includes
#include "scripting_handle.h"
#include "scripting.h"   // SR_LUA (Phase 17)
#include <src/database/database_pending.h>

#define H_HANDLE_METATABLE "H.Handle"

/*
 * __gc metamethod: release the handle when the userdata is collected.
 * Lua passes the userdata as argument 1. We pull the H_Handle pointer
 * out via the userdata's typed-pointer slot, then call H_Handle_release
 * (Phase 17 refcounted path). The HTTP worker thread holds a second
 * ref while the call is in flight, so the handle is only freed after
 * both the Lua userdata and the worker are done with it.
 */
int H_Handle_gc(lua_State* L) {
    H_Handle** ud = (H_Handle**)lua_touserdata(L, 1);
    if (ud && *ud) {
        H_Handle_release(*ud);
        *ud = NULL;
    }
    return 0;
}

void H_Handle_install_metatable(lua_State* L) {
    if (!L) {
        return;
    }

    // If already installed, do nothing. luaL_newmetatable creates the
    // metatable and returns 1 the first time, 0 on subsequent calls
    // (when the metatable already exists in the registry).
    if (luaL_newmetatable(L, H_HANDLE_METATABLE)) {
        // Push __gc = H_Handle_gc
        lua_pushcfunction(L, H_Handle_gc);
        lua_setfield(L, -2, "__gc");
        // Set the metatable's __metatable field to a string so
        // attempts to access the metatable from Lua raise an error
        // ("cannot access protected metatable"). This protects the
        // H_Handle pointer from Lua-side tampering.
        lua_pushliteral(L, "H.Handle is protected");
        lua_setfield(L, -2, "__metatable");
    }
    lua_pop(L, 1);
}

H_Handle* H_Handle_new(lua_State* L, H_HandleKind kind) {
    if (!L) {
        return NULL;
    }

    // Allocate userdata: a pointer-sized slot that will hold the
    // H_Handle pointer. The userdata itself is one pointer; the
    // H_Handle struct is a separate heap allocation.
    H_Handle** ud = (H_Handle**)lua_newuserdata(L, sizeof(H_Handle*));
    if (!ud) {
        return NULL;
    }

    // Attach the H.Handle metatable so __gc is wired up.
    luaL_getmetatable(L, H_HANDLE_METATABLE);
    if (!lua_istable(L, -1)) {
        // Metatable missing — install it now as a fallback.
        lua_pop(L, 1);
        H_Handle_install_metatable(L);
        luaL_getmetatable(L, H_HANDLE_METATABLE);
    }
    lua_setmetatable(L, -2);

    H_Handle* h = (H_Handle*)calloc(1, sizeof(H_Handle));
    if (!h) {
        log_this(SR_LUA, "H_Handle_new: allocation failed",
                 LOG_LEVEL_ERROR, 0);
        // Pop the userdata so the stack is clean.
        lua_pop(L, 1);
        return NULL;
    }
    h->kind = kind;
    h->consumed = false;
    h->refcount = 1;
    h->refcount_mutex_initialized = false;
    if (pthread_mutex_init(&h->refcount_mutex, NULL) != 0) {
        log_this(SR_LUA, "H_Handle_new: refcount mutex_init failed",
                 LOG_LEVEL_ERROR, 0);
        free(h);
        lua_pop(L, 1);
        return NULL;
    }
    h->refcount_mutex_initialized = true;
    h->query_id = NULL;
    h->db_queue = NULL;
    h->pending_query = NULL;
    h->error = NULL;
    h->http_url = NULL;
    h->http_method = NULL;
    h->http_body = NULL;
    h->http_content_type = NULL;
    h->http_timeout = 0;
    h->http_headers_slist = NULL;
    h->http_ready = false;
    h->http_cancelled = false;
    h->http_initialized = false;
    h->http_result_status = 0;
    h->http_result_headers_json = NULL;
    h->http_result_body = NULL;
    h->http_result_error = NULL;
    h->http_result_elapsed_ms = 0;
    // Phase 18: LLM kind fields
    h->llm_model_name = NULL;
    h->llm_prompt = NULL;
    h->llm_max_tokens = 0;
    h->llm_temperature = 0.0;
    h->llm_db_name = NULL;
    h->llm_result = NULL;
    h->llm_result_json = NULL;
    h->llm_error = NULL;
    h->llm_timeout = 0;
    h->llm_list = false;
    // Phase 19 / 7A: Mail and Notify fields
    h->mail_error = NULL;
    h->mail_message_id = NULL;
    h->mail_status = NULL;
    h->notify_error = NULL;

    // Phase 17: initialize the per-handle sync primitive for HTTP
    // kind. The QUERY kind does not need a condvar (it uses the
    // database queue's own condvar via database_queue_await_result).
    if (kind == H_HK_HTTP) {
        if (pthread_mutex_init(&h->http_mutex, NULL) != 0) {
            log_this(SR_LUA, "H_Handle_new: mutex_init failed",
                     LOG_LEVEL_ERROR, 0);
            free(h);
            lua_pop(L, 1);
            return NULL;
        }
        if (pthread_cond_init(&h->http_cond, NULL) != 0) {
            log_this(SR_LUA, "H_Handle_new: cond_init failed",
                     LOG_LEVEL_ERROR, 0);
            pthread_mutex_destroy(&h->http_mutex);
            free(h);
            lua_pop(L, 1);
            return NULL;
        }
        h->http_initialized = true;
    }

    *ud = h;
    return h;
}

void H_Handle_free(H_Handle* h) {
    if (!h) {
        return;
    }
    /*
     * Drop a still-registered pending waiter when the handle is freed
     * without H.wait (GC, error path). Prevents orphan entries that
     * block cleanup_global_pending_manager during database landing.
     */
    if (h->pending_query) {
        PendingResultManager* mgr = get_pending_result_manager();
        if (mgr) {
            pending_result_unregister(mgr, h->pending_query, SR_SCRIPTING);
        }
        h->pending_query = NULL;
    }
    if (h->query_id) {
        free(h->query_id);
        h->query_id = NULL;
    }
    if (h->error) {
        free(h->error);
        h->error = NULL;
    }
    if (h->http_url) {
        free(h->http_url);
        h->http_url = NULL;
    }
    if (h->http_method) {
        free(h->http_method);
        h->http_method = NULL;
    }
    if (h->http_body) {
        free(h->http_body);
        h->http_body = NULL;
    }
    if (h->http_content_type) {
        free(h->http_content_type);
        h->http_content_type = NULL;
    }
    if (h->http_headers_slist) {
        curl_slist_free_all((struct curl_slist*)h->http_headers_slist);
        h->http_headers_slist = NULL;
    }
    if (h->http_result_headers_json) {
        free(h->http_result_headers_json);
        h->http_result_headers_json = NULL;
    }
    if (h->http_result_body) {
        free(h->http_result_body);
        h->http_result_body = NULL;
    }
    if (h->http_result_error) {
        free(h->http_result_error);
        h->http_result_error = NULL;
    }
    // Phase 18: free LLM kind fields
    if (h->llm_model_name) {
        free(h->llm_model_name);
        h->llm_model_name = NULL;
    }
    if (h->llm_prompt) {
        free(h->llm_prompt);
        h->llm_prompt = NULL;
    }
    if (h->llm_db_name) {
        free(h->llm_db_name);
        h->llm_db_name = NULL;
    }
    if (h->llm_result) {
        free(h->llm_result);
        h->llm_result = NULL;
    }
    if (h->llm_result_json) {
        free(h->llm_result_json);
        h->llm_result_json = NULL;
    }
    if (h->llm_error) {
        free(h->llm_error);
        h->llm_error = NULL;
    }
    // Phase 19 / 7A: free Mail and Notify fields
    if (h->mail_error) {
        free(h->mail_error);
        h->mail_error = NULL;
    }
    if (h->mail_message_id) {
        free(h->mail_message_id);
        h->mail_message_id = NULL;
    }
    if (h->mail_status) {
        free(h->mail_status);
        h->mail_status = NULL;
    }
    if (h->notify_error) {
        free(h->notify_error);
        h->notify_error = NULL;
    }
    // Phase 17: destroy the per-handle sync primitive. Only do this
    // for HTTP kind handles (the QUERY kind's http_initialized is
    // false, so the destroy is skipped).
    if (h->http_initialized) {
        pthread_cond_destroy(&h->http_cond);
        pthread_mutex_destroy(&h->http_mutex);
        h->http_initialized = false;
    }
    if (h->refcount_mutex_initialized) {
        pthread_mutex_destroy(&h->refcount_mutex);
        h->refcount_mutex_initialized = false;
    }
    // db_queue is not owned by the handle.
    h->db_queue = NULL;
    free(h);
}

/*
 * Phase 17: bump the refcount. Used by the HTTP worker thread to
 * keep the handle alive while the call is in flight. The Lua
 * userdata holds one ref (released by __gc -> H_Handle_release);
 * the worker holds a second ref while it owns the handle.
 */
void H_Handle_acquire(H_Handle* h) {
    if (!h) {
        return;
    }
    if (h->refcount_mutex_initialized) {
        pthread_mutex_lock(&h->refcount_mutex);
        h->refcount++;
        pthread_mutex_unlock(&h->refcount_mutex);
    } else {
        h->refcount++;
    }
}

/*
 * Phase 17: drop a ref. Frees the handle when the refcount hits 0.
 * Used by __gc and by the HTTP worker thread after it stores the
 * result. No-op on NULL.
 */
void H_Handle_release(H_Handle* h) {
    if (!h) {
        return;
    }
    int remaining;
    if (h->refcount_mutex_initialized) {
        pthread_mutex_lock(&h->refcount_mutex);
        if (h->refcount > 0) {
            h->refcount--;
        }
        remaining = h->refcount;
        pthread_mutex_unlock(&h->refcount_mutex);
    } else {
        if (h->refcount > 0) {
            h->refcount--;
        }
        remaining = h->refcount;
    }
    if (remaining == 0) {
        H_Handle_free(h);
    }
}

/*
 * Phase 17: thread-safe read of the current refcount. Useful for
 * tests that need to observe the in-flight/worker-done boundary.
 */
int H_Handle_get_refcount(H_Handle* h) {
    if (!h) {
        return 0;
    }
    if (h->refcount_mutex_initialized) {
        pthread_mutex_lock(&h->refcount_mutex);
        int n = h->refcount;
        pthread_mutex_unlock(&h->refcount_mutex);
        return n;
    }
    return h->refcount;
}

H_Handle* H_Handle_check(lua_State* L, int arg) {
    if (!L) {
        return NULL;
    }
    // luaL_checkudata raises a Lua error on type mismatch, which
    // would unwind the calling C function. We don't want that for
    // H.wait because the error should be returned as a string per
    // the Phase 2 contract. So use lua_touserdata + manual check.
    void* ud = lua_touserdata(L, arg);
    if (!ud) {
        return NULL;
    }
    // Confirm the userdata has the H.Handle metatable.
    if (!lua_getmetatable(L, arg)) {
        return NULL;
    }
    luaL_getmetatable(L, H_HANDLE_METATABLE);
    int same = lua_rawequal(L, -1, -2);
    lua_pop(L, 2);
    if (!same) {
        return NULL;
    }
    H_Handle** slot = (H_Handle**)ud;
    return *slot;
}
