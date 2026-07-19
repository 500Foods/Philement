/*
 * Mock chat dependencies for auth_chat success-path unit testing.
 * See mock_auth_chat_deps.h for details.
 */

#include "mock_auth_chat_deps.h"
#include <stdlib.h>
#include <string.h>

// Static state
static bool g_proxy_success = true;
static char* g_proxy_body = NULL;
static bool g_segment_exists = true;
static uint64_t g_cache_hits = 10;
static uint64_t g_cache_misses = 2;
static double g_cache_ratio = 0.833;
static int g_proxy_call_count = 0;
static int g_store_segment_count = 0;
static bool g_media_resolve_success = false;
static char* g_media_resolved_json = NULL;
static char* g_media_error_message = NULL;
static int g_multi_failure_index = -1;

ChatProxyResult* mock_chat_proxy_send_with_retry(const ChatEngineConfig* engine,
                                                  const char* request_json,
                                                  const ChatProxyConfig* config) {
    (void)engine;
    (void)request_json;
    (void)config;
    g_proxy_call_count++;
    ChatProxyResult* result = calloc(1, sizeof(ChatProxyResult));
    if (!result) return NULL;

    if (g_proxy_success) {
        result->code = CHAT_PROXY_OK;
        result->http_status = 200;
        result->response_body = g_proxy_body ? strdup(g_proxy_body) : NULL;
        result->response_size = result->response_body ? strlen(result->response_body) : 0;
        result->error_message = NULL;
    } else {
        result->code = CHAT_PROXY_ERROR_UNKNOWN;
        result->http_status = 502;
        result->response_body = NULL;
        result->response_size = 0;
        result->error_message = strdup("mock proxy failure");
    }
    return result;
}

ChatMultiResult* mock_chat_proxy_send_multi(const ChatMultiRequest* requests,
                                             size_t request_count,
                                             const ChatProxyConfig* config) {
    (void)requests;
    (void)config;
    ChatMultiResult* multi_result = chat_multi_result_create(request_count);
    if (!multi_result) return NULL;

    multi_result->total_time_ms = 12.5;
    for (size_t i = 0; i < request_count; i++) {
        g_proxy_call_count++;
        ChatProxyResult* result = calloc(1, sizeof(ChatProxyResult));
        if (!result) continue;
        result->total_time_ms = 5.0 + (double)i;
        if ((int)i == g_multi_failure_index || !g_proxy_success) {
            result->code = CHAT_PROXY_ERROR_UNKNOWN;
            result->http_status = 502;
            result->error_message = strdup("mock proxy failure");
            multi_result->failure_count++;
        } else {
            result->code = CHAT_PROXY_OK;
            result->http_status = 200;
            result->response_body = g_proxy_body ? strdup(g_proxy_body) : NULL;
            result->response_size = result->response_body ? strlen(result->response_body) : 0;
            multi_result->success_count++;
        }
        multi_result->results[i] = result;
    }
    return multi_result;
}

char* mock_chat_storage_store_segment(const char* database, const char* message, size_t message_len) {
    (void)database;
    (void)message;
    (void)message_len;
    g_store_segment_count++;
    return strdup("seg_hash_0001");
}

bool mock_chat_storage_segment_exists(const char* database, const char* segment_hash) {
    (void)database;
    (void)segment_hash;
    return g_segment_exists;
}

bool mock_chat_storage_cache_get_stats(const char* database,
                                        uint64_t* hits, uint64_t* misses, double* hit_ratio) {
    (void)database;
    if (hits) *hits = g_cache_hits;
    if (misses) *misses = g_cache_misses;
    if (hit_ratio) *hit_ratio = g_cache_ratio;
    return true;
}

char* mock_chat_storage_store_chat(const char* database, const char* convos_ref,
                                    const char** segment_hashes, size_t hash_count,
                                    const char* engine_name, const char* model,
                                    int tokens_prompt, int tokens_completion,
                                    double cost_total, int user_id, int duration_ms,
                                    const char* session_id) {
    (void)database;
    (void)convos_ref;
    (void)segment_hashes;
    (void)hash_count;
    (void)engine_name;
    (void)model;
    (void)tokens_prompt;
    (void)tokens_completion;
    (void)cost_total;
    (void)user_id;
    (void)duration_ms;
    (void)session_id;
    return strdup("chat_stored_ok");
}

void mock_chat_storage_free_hash(char* hash) {
    free(hash);
}

bool mock_chat_storage_resolve_media_in_content(const char* database,
                                                 const char* content_json,
                                                 char** resolved_json,
                                                 char** error_message) {
    (void)database;
    (void)content_json;
    if (resolved_json) {
        *resolved_json = g_media_resolved_json ? strdup(g_media_resolved_json) : NULL;
    }
    if (error_message) {
        *error_message = g_media_error_message ? strdup(g_media_error_message) : NULL;
    }
    return g_media_resolve_success;
}

void mock_auth_chat_deps_set_proxy_success(bool success) {
    g_proxy_success = success;
}

void mock_auth_chat_deps_set_proxy_response_body(const char* body) {
    free(g_proxy_body);
    g_proxy_body = body ? strdup(body) : NULL;
}

void mock_auth_chat_deps_set_multi_failure_index(int failure_index) {
    g_multi_failure_index = failure_index;
}

void mock_auth_chat_deps_set_segment_exists(bool exists) {
    g_segment_exists = exists;
}

void mock_auth_chat_deps_set_cache_stats(uint64_t hits, uint64_t misses, double hit_ratio) {
    g_cache_hits = hits;
    g_cache_misses = misses;
    g_cache_ratio = hit_ratio;
}

void mock_auth_chat_deps_set_media_resolve(bool success, const char* resolved_json, const char* error_message) {
    g_media_resolve_success = success;
    free(g_media_resolved_json);
    free(g_media_error_message);
    g_media_resolved_json = resolved_json ? strdup(resolved_json) : NULL;
    g_media_error_message = error_message ? strdup(error_message) : NULL;
}

int mock_auth_chat_deps_proxy_call_count(void) {
    return g_proxy_call_count;
}

int mock_auth_chat_deps_store_segment_count(void) {
    return g_store_segment_count;
}

void mock_auth_chat_deps_reset_all(void) {
    g_proxy_success = true;
    free(g_proxy_body);
    g_proxy_body = NULL;
    g_segment_exists = true;
    g_cache_hits = 10;
    g_cache_misses = 2;
    g_cache_ratio = 0.833;
    g_proxy_call_count = 0;
    g_store_segment_count = 0;
    g_media_resolve_success = false;
    free(g_media_resolved_json);
    g_media_resolved_json = NULL;
    free(g_media_error_message);
    g_media_error_message = NULL;
    g_multi_failure_index = -1;
}
