/*
 * Scripting Subsystem - HTTP Worker Pool
 *
 * Phase 17 of the LUA_PLAN. A small pool of background threads that
 * perform the blocking libcurl call for H.http.* requests. The
 * calling Lua thread parks on the handle's per-handle condvar and
 * is woken when the result is stored. This lets multiple H.http.get
 * calls fan out in parallel.
 *
 * Design (mirrors the existing scripting worker pool):
 *   - N pthreads that loop, dequeue an H_Handle* from a Queue*,
 *     perform the HTTP call via scripting_http_get/_post, store
 *     the result in the handle's per-handle fields, signal the
 *     condvar, and loop.
 *   - The handle owns its own result fields and sync primitive
 *     (mutex + condvar + ready flag). The worker thread acquires
 *     a refcount on the handle so the handle cannot be freed
 *     while the call is in flight (H_Handle_release from the
 *     worker drops the ref).
 *   - The queue holds opaque H_Handle* pointers (cast through
 *     a heap-allocated wrapper so the queue's free callback is
 *     correct).
 *   - Shutdown: set the shutdown flag, NULL the global, join
 *     workers (drain-on-shutdown matches the scripting worker
 *     pool), clear and release the queue.
 *
 * Concurrency: workers run independently on their own pthreads.
 * The handle's mutex protects the per-handle result fields; the
 * condvar is signaled by the worker when the result is stored.
 *
 * @see docs/H/plans/complete/LUA_PLAN_COMPLETE.md Phase 17
 */

#ifndef HYDROGEN_SCRIPTING_HTTP_POOL_H
#define HYDROGEN_SCRIPTING_HTTP_POOL_H

// System headers
#include <stdbool.h>
#include <stddef.h>

// Project headers
#include <pthread.h>
#include <src/queue/queue.h>
#include "scripting_handle.h"   // H_Handle
#include <src/api/auth/oidc_rp/oidc_rp_http.h>   // OidcRpHttpResponse

#define SCRIPTING_HTTP_QUEUE_NAME "scripting_http"

/*
 * The HTTP worker pool. One queue, N pthreads, no script registry
 * (HTTP calls do not need a registry - the handle carries the
 * request). Allocated by scripting_http_pool_init() and freed by
 * scripting_http_pool_destroy().
 */
typedef struct ScriptingHttpPool {
    Queue*       http_queue;
    pthread_t*   worker_tids;
    int          worker_count;
} ScriptingHttpPool;

// Global HTTP pool pointer. NULL when the subsystem is not running.
extern ScriptingHttpPool* scripting_http_pool;

/*
 * Initialize the HTTP worker pool. Creates the queue and starts
 * `worker_count` pthreads. Each pthread is registered with
 * ServiceThreads (scripting_threads) with a description like
 * "http-worker-0", "http-worker-1", ...
 *
 * Idempotent: calling twice is a no-op the second time.
 *
 * Returns true on success, false on allocation or pthread failure
 * (in which case any partially-created state is torn down).
 */
bool scripting_http_pool_init(int worker_count);

/*
 * Signal shutdown, drain the queue, join all worker threads, and
 * free the pool. Safe to call with NULL pool (no-op). Idempotent.
 *
 * After this call scripting_http_pool is NULL.
 */
void scripting_http_pool_destroy(void);

/*
 * Submit an HTTP handle to the pool. The pool will perform the
 * HTTP call on a background thread and store the result in the
 * handle. The caller (H_lua_http_wait_one) blocks on the
 * handle's condvar until the result is ready.
 *
 * Acquires a refcount on the handle so the handle cannot be
 * freed while the call is in flight. The worker releases the
 * ref after storing the result.
 *
 * Returns true on success, false if the pool is not running or
 * the enqueue failed.
 */
bool scripting_http_pool_submit(H_Handle* h);

/*
 * Exposed for Unity tests (NOT part of the stable public API).
 */
char* serialize_headers(const OidcRpHttpResponse* resp);
void* scripting_http_worker_thread(void* arg);
void scripting_http_worker_process_one(ScriptingHttpPool* pool,
                                      H_Handle* h);
bool scripting_http_worker_should_exit(ScriptingHttpPool* pool);

#endif /* HYDROGEN_SCRIPTING_HTTP_POOL_H */
