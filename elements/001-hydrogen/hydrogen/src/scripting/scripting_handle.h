/*
 * Scripting Subsystem - H_Handle (Async Result Handle)
 *
 * Phase 13 of the LUA_PLAN. The H_Handle is an opaque Lua userdata
 * that wraps a pending async operation (currently: a database query
 * submitted via the queue layer).
 *
 * Lifecycle:
 *   1. H.query / H.altquery / H.authquery creates an H_Handle,
 *      submits the query to the database queue, and returns a Lua
 *      userdata that wraps the H_Handle pointer.
 *   2. The Lua script may do other work, then call H.wait(handle).
 *   3. H.wait calls database_queue_await_result, parses the result
 *      JSON into a Lua table, and marks the handle consumed.
 *   4. The handle's __gc metamethod frees the H_Handle and any
 *      remaining state (the query_id, any pre-set error string).
 *
 * Handle structure:
 *   - kind          H_HK_QUERY (the only kind in Phase 13)
 *   - consumed      1 after H.wait has returned its result/error
 *                   for this handle. A second H.wait on a consumed
 *                   handle returns nil + "handle already consumed".
 *   - query_id      The string ID used to track the pending result.
 *                   NULL for handles that failed before submission.
 *   - db_queue      Pointer to the DatabaseQueue the query was
 *                   submitted to. NULL for pre-submission failures.
 *   - error         Pre-set error string (e.g. "no DefaultDatabase
 *                   configured", "submit failed"). When non-NULL, the
 *                   handle is already in an error state and H.wait
 *                   will return it without touching the queue.
 *
 * Memory ownership:
 *   - H_Handle is heap-allocated by H_Handle_new and freed by the
 *     __gc metamethod (or by H_Handle_free directly in error paths).
 *   - query_id is a strdup'd copy owned by the handle.
 *   - error is a strdup'd copy owned by the handle.
 *   - db_queue is NOT owned by the handle (the queue manager owns it).
 *
 * UAF discipline (Phase 1): the C side never holds Lua-owned memory
 * after a host function returns. The userdata is allocated by C and
 * freed by C; the Lua side only holds an opaque pointer.
 */

#ifndef HYDROGEN_SCRIPTING_SCRIPTING_HANDLE_H
#define HYDROGEN_SCRIPTING_SCRIPTING_HANDLE_H

// System headers
#include <stdbool.h>

// Third-party headers
#include <lua.h>

// Project headers
#include <pthread.h>   // pthread_mutex_t, pthread_cond_t (Phase 17)
#include <src/database/dbqueue/dbqueue.h>   // DatabaseQueue

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Handle kinds. The kind identifies how H.wait will resolve the
 * handle. Phase 13 added H_HK_QUERY; Phase 16 adds H_HK_HTTP;
 * Phase 18 adds H_HK_LLM; Phase 19 adds H_HK_MAIL, H_HK_NOTIFY.
 */
typedef enum {
    H_HK_QUERY = 1,
    H_HK_HTTP  = 2,
    H_HK_LLM   = 3,
    H_HK_MAIL  = 4,
    H_HK_NOTIFY = 5
} H_HandleKind;

/* Forward decls for the per-handle sync primitive (Phase 17). */
struct H_Handle;

/*
 * Phase 17: refcounted handle. The Lua userdata holds one ref
 * (released by __gc). The HTTP worker thread acquires a second
 * ref before processing and releases when done, so the handle
 * cannot be freed while the background thread is using it. H_Handle_new
 * initializes refcount to 1.
 *
 * The refcount is protected by its own mutex so acquire/release/get are
 * safe to call from any thread.
 */
void H_Handle_acquire(struct H_Handle* h);
void H_Handle_release(struct H_Handle* h);
int H_Handle_get_refcount(struct H_Handle* h);

/*
 * The C-side handle backing the Lua userdata. Allocated by
 * H_Handle_new, freed by H_Handle_free (also wired to __gc).
 *
 * Field use by kind:
 *   H_HK_QUERY:
 *     query_id      strdup'd, owned; NULL for pre-submit failures
 *     db_queue      not owned; NULL for pre-submit failures
 *     error         strdup'd; non-NULL means handle is in an error state
 *     http_url,
 *     http_method,
 *     http_body,
 *     http_headers_json  unused (NULL)
 *   H_HK_HTTP:
 *     http_url      strdup'd, owned (the request URL)
 *     http_method   strdup'd, owned ("GET" or "POST")
 *     http_body     strdup'd, owned (POST body, may be NULL for GET)
 *     http_content_type   strdup'd, owned (HTTP) - may be NULL (POST only)
 *     http_timeout        int, seconds; <=0 means "use config default" (HTTP)
 *     http_headers_slist    opaque pointer to the curl_slist built from the Lua headers table (HTTP)
 *     query_id, db_queue, error  unused (NULL)
*   H_HK_LLM:
 *     llm_model_name    strdup'd, owned (the model name, e.g. "grok-light")
 *     llm_prompt        strdup'd, owned (the user prompt, may be NULL for list)
 *     llm_max_tokens    int (0 = use default)
 *     llm_temperature   double (0 = use default)
 *     llm_db_name       strdup'd, owned (the database for model resolution, may be NULL)
 *     llm_result        strdup'd, owned (the response text)
 *     llm_error         strdup'd; non-NULL means handle is in an error state (LLM)
 *     llm_timeout       seconds; 0 = use default (LLM)
 *     llm_list          true if this is a list operation (LLM)
 *   H_HK_MAIL:
 *     mail_error        strdup'd; non-NULL means handle is in an error state (MAIL stub)
 *   H_HK_NOTIFY:
 *     notify_error      strdup'd; non-NULL means handle is in an error state (NOTIFY stub)
 */
typedef struct H_Handle {
    H_HandleKind    kind;
    bool            consumed;     // true after H.wait returns
    int             refcount;     // Phase 17: 1 at creation, ++/-- by acquire/release
    bool            refcount_mutex_initialized; // true once refcount_mutex is valid
    pthread_mutex_t refcount_mutex; // protects refcount across threads
    char*           query_id;     // strdup'd; NULL for pre-submit failures (QUERY)
    DatabaseQueue*  db_queue;     // not owned; NULL for pre-submit failures (QUERY)
    char*           error;        // strdup'd; non-NULL means handle is in error state
    // Phase 16: HTTP kind fields
    char*           http_url;            // strdup'd; owned (HTTP)
    char*           http_method;         // strdup'd; owned (HTTP) - "GET" or "POST"
    char*           http_body;           // strdup'd; owned (HTTP) - may be NULL
    char*           http_content_type;   // strdup'd; owned (HTTP) - may be NULL (POST only)
    int             http_timeout;        // seconds; <=0 means "use config default" (HTTP)
    void*           http_headers_slist;  // struct curl_slist* (HTTP)
    // Phase 17: per-handle sync primitive (HTTP kind only). The HTTP worker thread writes the result into the handle and signals http_cond; H.wait blocks on http_cond. Mutex/cond are initialized lazily by H_Handle_new for HTTP kind and destroyed by H_Handle_free.
    pthread_mutex_t http_mutex;          // protects the fields below (HTTP)
    pthread_cond_t  http_cond;           // signaled when http_ready or http_cancelled is set
    bool            http_ready;          // true once the worker has stored the result
    bool            http_cancelled;      // true if the handle was freed before wait
    bool            http_initialized;    // true once mutex/cond have been initialized
    int             http_result_status;  // HTTP status (HTTP)
    char*           http_result_headers_json;  // strdup'd, JSON-encoded headers table (HTTP)
    char*           http_result_body;    // strdup'd, response body (HTTP)
    char*           http_result_error;   // strdup'd, error string (NULL on success) (HTTP)
    long            http_result_elapsed_ms;     // elapsed time of the HTTP call (HTTP)
    // Phase 18: LLM kind fields
    char*           llm_model_name;      // strdup'd; owned (LLM)
    char*           llm_prompt;          // strdup'd; owned (LLM) - may be NULL for list
    int             llm_max_tokens;      // int; 0 = use default (LLM)
    double          llm_temperature;     // double; 0 = use default (LLM)
    char*           llm_db_name;         // strdup'd; owned (LLM) - database for model resolution
    char*           llm_result;          // strdup'd; owned (LLM) - response text
    char*           llm_result_json;     // strdup'd; owned (LLM) - full JSON response (optional)
    char*           llm_error;           // strdup'd; non-NULL means handle is in error state (LLM)
    int             llm_timeout;         // seconds; 0 = use default (LLM)
    bool            llm_list;            // true if this is a list operation (LLM)
    // Phase 19: Mail and Notify stub fields
    char*           mail_error;          // strdup'd; non-NULL means handle is in an error state (MAIL stub)
    char*           notify_error;        // strdup'd; non-NULL means handle is in an error state (NOTIFY stub)
} H_Handle;

/*
 * Create a new H_Handle. Returns a Lua userdata on the stack wrapping
 * the H_Handle pointer. The userdata has the H.Handle metatable
 * installed (with __gc). Returns NULL and pushes nothing if allocation
 * fails (caller should check the stack).
 *
 * Fields default to: kind = kind, consumed = false, query_id = NULL,
 * db_queue = NULL, error = NULL. The caller fills them in after
 * creation.
 */
H_Handle* H_Handle_new(lua_State* L, H_HandleKind kind);

/*
 * Free an H_Handle and its owned strings. Safe with NULL. Wired to
 * the H.Handle metatable's __gc entry. Idempotent (sets handle fields
 * to safe values after free).
 */
void H_Handle_free(H_Handle* h);

/*
 * Install the H.Handle metatable on the Lua state. Must be called
 * once per lua_State, before any handle is created. The metatable
 * is stored in the registry under "H.Handle" so it is shared across
 * all handles.
 *
 * Idempotent: re-calling is a no-op.
 */
void H_Handle_install_metatable(lua_State* L);

/*
 * Validate a Lua argument as an H_Handle userdata. Returns a pointer
 * to the H_Handle (caller does NOT own) or NULL if the argument is
 * not a valid handle. Pushes an error string when the argument is
 * the wrong type so the caller can forward it as H.wait's error.
 */
H_Handle* H_Handle_check(lua_State* L, int arg);

#ifdef __cplusplus
}
#endif

#endif /* HYDROGEN_SCRIPTING_SCRIPTING_HANDLE_H */
