/*
 * Scripting Subsystem - HTTP Worker Pool
 *
 * Phase 17 of the LUA_PLAN. See http_pool.h for the design.
 */

 // Project includes
#include <src/hydrogen.h>

 // Standard includes
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Third-party includes
#include <jansson.h>   // json_object_update_recursive, json_array, etc.
#include <curl/curl.h>   // curl_slist_free_all (Phase 17)

// Local includes
#include "scripting.h"
#include "http_pool.h"
#include "http_client.h"
#include "scripting_handle.h"
#include <src/threads/threads.h>
#include <src/api/auth/oidc_rp/oidc_rp_http.h>   // OidcRpHttpResponse, free

extern ServiceThreads scripting_threads;
extern volatile sig_atomic_t scripting_system_shutdown;

// The HTTP pool global. NULL when the subsystem is not running.
ScriptingHttpPool* scripting_http_pool = NULL;

// Idle sleep between empty queue polls (microseconds).
#define HTTP_POOL_POLL_USEC 1000

// Forward declarations.
static void* scripting_http_worker_thread(void* arg);
static void scripting_http_worker_process_one(ScriptingHttpPool* pool,
                                               H_Handle* h);
static bool scripting_http_worker_should_exit(ScriptingHttpPool* pool);

/*
 * The pool's queue holds opaque pointers. We wrap each H_Handle*
 * in a small heap struct and enqueue the struct's content (which
 * is just the pointer value). The queue copies the struct's bytes
 * (not the handle's bytes), so the worker reads back the correct
 * pointer. The wrapper is freed by the worker after processing.
 */
typedef struct HttpPoolItem {
    H_Handle* handle;
} HttpPoolItem;

/*
 * Serialize the headers array of an OidcRpHttpResponse into a JSON
 * string for storage on the handle. Returns a strdup'd string or
 * NULL on failure / no headers.
 */
static char* serialize_headers(const OidcRpHttpResponse* resp) {
    if (!resp || resp->headers_count == 0) {
        return NULL;
    }
    json_t* arr = json_array();
    if (!arr) {
        return NULL;
    }
    for (size_t i = 0; i < resp->headers_count; i++) {
        const char* name = resp->headers[i].name ? resp->headers[i].name : "";
        const char* value = resp->headers[i].value ? resp->headers[i].value : "";
        json_t* obj = json_pack("{s:s, s:s}", "name", name, "value", value);
        if (obj) {
            json_array_append_new(arr, obj);
        }
    }
    char* out = json_dumps(arr, JSON_COMPACT);
    json_decref(arr);
    return out;
}

/*
 * Worker thread main loop. Each worker is independent; no worker
 * shares a handle with any other.
 */
static void* scripting_http_worker_thread(void* arg) {
    ScriptingHttpPool* pool = (ScriptingHttpPool*)arg;

    while (1) {
        int prio = 0;
        size_t sz = 0;
        HttpPoolItem* item = (HttpPoolItem*)queue_dequeue_nonblocking(
            pool->http_queue, &sz, &prio);
        if (!item) {
            if (scripting_http_worker_should_exit(pool)) {
                return NULL;
            }
            usleep(HTTP_POOL_POLL_USEC);
            continue;
        }

        scripting_http_worker_process_one(pool, item->handle);
        free(item);
    }
}

/*
 * Process one HTTP request. Stores the result in the handle's
 * per-handle fields and signals the condvar. Releases the ref
 * acquired by scripting_http_pool_submit when done.
 */
static void scripting_http_worker_process_one(ScriptingHttpPool* pool,
                                               H_Handle* h) {
    (void)pool;

    if (!h) {
        return;
    }

    // Take ownership of the headers slist (scripting_http_get/_post
    // pass it to the OIDC helper, which frees it on return).
    struct curl_slist* headers = (struct curl_slist*)h->http_headers_slist;
    h->http_headers_slist = NULL;

    struct timespec start_ts;
    clock_gettime(CLOCK_MONOTONIC, &start_ts);

    struct OidcRpHttpResponse* resp = NULL;
    if (strcmp(h->http_method, "GET") == 0) {
        resp = scripting_http_get(h->http_url, headers,
                                  h->http_timeout, true);
    } else if (strcmp(h->http_method, "POST") == 0) {
        resp = scripting_http_post(h->http_url, h->http_body,
                                   h->http_content_type, headers,
                                   h->http_timeout, true);
    } else {
        // Should not happen. Free the slist we claimed above.
        if (headers) curl_slist_free_all(headers);
        pthread_mutex_lock(&h->http_mutex);
        h->http_result_error = strdup("H.wait: unknown HTTP method on handle");
        h->http_ready = true;
        pthread_cond_broadcast(&h->http_cond);
        pthread_mutex_unlock(&h->http_mutex);
        H_Handle_release(h);
        return;
    }

    struct timespec end_ts;
    clock_gettime(CLOCK_MONOTONIC, &end_ts);
    long elapsed_ms = (long)(((end_ts.tv_sec - start_ts.tv_sec) * 1000L) +
                             ((end_ts.tv_nsec - start_ts.tv_nsec) / 1000000L));

    pthread_mutex_lock(&h->http_mutex);
    h->http_result_elapsed_ms = elapsed_ms;
    if (resp) {
        if (resp->error_message) {
            h->http_result_error = strdup(resp->error_message);
        } else {
            h->http_result_status = (int)resp->http_status;
            if (resp->body) {
                h->http_result_body = strdup(resp->body);
            }
            h->http_result_headers_json = serialize_headers(resp);
        }
        oidc_rp_http_response_free(resp);
    } else {
        h->http_result_error = strdup("H.wait: HTTP call returned no response");
    }
    h->http_ready = true;
    pthread_cond_broadcast(&h->http_cond);
    pthread_mutex_unlock(&h->http_mutex);

    // Release the ref acquired by scripting_http_pool_submit.
    H_Handle_release(h);
}

static bool scripting_http_worker_should_exit(ScriptingHttpPool* pool) {
    if (!scripting_system_shutdown) {
        return false;
    }
    if (!pool || !pool->http_queue) {
        return true;
    }
    return queue_size(pool->http_queue) == 0;
}

/*
 * Initialize the HTTP worker pool. See http_pool.h.
 */
bool scripting_http_pool_init(int worker_count) {
    if (scripting_http_pool) {
        return true;
    }
    if (worker_count < 1) {
        log_this(SR_SCRIPTING, "scripting_http_pool_init: invalid worker_count",
                 LOG_LEVEL_ERROR, 0);
        return false;
    }

    ScriptingHttpPool* pool = calloc(1, sizeof(ScriptingHttpPool));
    if (!pool) {
        return false;
    }

    pool->worker_count = worker_count;
    pool->worker_tids = calloc((size_t)worker_count, sizeof(pthread_t));
    QueueAttributes attrs = {0};
    pool->http_queue = queue_create_with_label(SCRIPTING_HTTP_QUEUE_NAME,
                                               &attrs, SR_SCRIPTING);

    if (!pool->worker_tids || !pool->http_queue) {
        log_this(SR_SCRIPTING, "scripting_http_pool_init: allocation failure",
                 LOG_LEVEL_ERROR, 0);
        if (pool->worker_tids) free(pool->worker_tids);
        if (pool->http_queue) queue_release(pool->http_queue);
        free(pool);
        return false;
    }

    // Take a reference for our own use.
    queue_acquire(pool->http_queue);

    // Defensive: clear any leftover items from a prior shutdown.
    queue_clear(pool->http_queue);

    // Reset the shutdown flag so this pool is active. A previous
    // destroy may have set it to 1; without this reset, the
    // workers' should_exit() returns true on their first idle
    // iteration and they exit before processing anything.
    scripting_system_shutdown = 0;

    // Publish the pool BEFORE starting the workers. The workers'
    // should_exit() checks scripting_http_pool, so it must be non-NULL
    // by the time they run their first loop iteration.
    scripting_http_pool = pool;

    bool ok = true;
    for (int i = 0; i < worker_count; i++) {
        if (pthread_create(&pool->worker_tids[i], NULL,
                           scripting_http_worker_thread, pool) != 0) {
            log_this(SR_SCRIPTING, "scripting_http_pool_init: pthread_create failed",
                     LOG_LEVEL_ERROR, 0);
            ok = false;
            break;
        }
        char desc[32];
        snprintf(desc, sizeof(desc), "http-worker-%d", i);
        add_service_thread_with_description(&scripting_threads,
                                            pool->worker_tids[i], desc);
    }

    if (!ok) {
        scripting_system_shutdown = 1;
        scripting_http_pool = NULL;
        for (int i = 0; i < worker_count; i++) {
            if (pool->worker_tids[i]) {
                pthread_join(pool->worker_tids[i], NULL);
                remove_service_thread(&scripting_threads, pool->worker_tids[i]);
            }
        }
        queue_release(pool->http_queue);
        free(pool->worker_tids);
        free(pool);
        return false;
    }

    log_this(SR_SCRIPTING, "HTTP pool started: %d worker(s)",
             LOG_LEVEL_STATE, 1, worker_count);
    return true;
}

/*
 * Signal shutdown, drain, join, and free.
 */
void scripting_http_pool_destroy(void) {
    ScriptingHttpPool* pool = scripting_http_pool;
    if (!pool) {
        return;
    }

    // Set the shutdown flag FIRST, then NULL the pool pointer. The
    // workers' should_exit() checks the shutdown flag AND an empty
    // queue, so setting shutdown first lets them drain the queue
    // before exiting. If we NULLed first, the workers' NULL check
    // would fire and they'd exit before draining.
    scripting_system_shutdown = 1;
    scripting_http_pool = NULL;

    for (int i = 0; i < pool->worker_count; i++) {
        pthread_join(pool->worker_tids[i], NULL);
        remove_service_thread(&scripting_threads, pool->worker_tids[i]);
    }

    if (pool->http_queue) {
        queue_clear(pool->http_queue);
        queue_release(pool->http_queue);
        pool->http_queue = NULL;
    }

    free(pool->worker_tids);
    free(pool);
    log_this(SR_SCRIPTING, "HTTP pool stopped", LOG_LEVEL_STATE, 0);
}

/*
 * Submit an HTTP handle to the pool. Acquires a refcount on the
 * handle so the handle cannot be freed while the call is in flight.
 */
bool scripting_http_pool_submit(H_Handle* h) {
    if (!scripting_http_pool || !h) {
        return false;
    }
    if (h->kind != H_HK_HTTP) {
        return false;
    }
    HttpPoolItem* item = calloc(1, sizeof(HttpPoolItem));
    if (!item) {
        return false;
    }
    H_Handle_acquire(h);
    item->handle = h;
    // Enqueue the wrapper struct (not the handle directly). The
    // queue copies sizeof(HttpPoolItem) bytes from item, which
    // contains the pointer value. The worker reads the pointer
    // back from the copied bytes and frees the wrapper.
    if (!queue_enqueue(scripting_http_pool->http_queue,
                       (const char*)item, sizeof(HttpPoolItem), 0)) {
        H_Handle_release(h);
        free(item);
        return false;
    }
    return true;
}
