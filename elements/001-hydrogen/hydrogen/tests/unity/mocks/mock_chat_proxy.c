/*
 * Mock chat proxy implementation for wschat health unit testing.
 *
 * Returns a caller-owned ChatProxyResult with a configurable HTTP status so
 * the chat health-check functions can be driven through every branch without a
 * live AI provider. The returned result is allocated with malloc and freed by
 * the matching mock_chat_proxy_result_destroy().
 */

#include "mock_chat_proxy.h"

#include <stdlib.h>
#include <string.h>

// Static mock state
static int g_http_status = 200;
static bool g_return_null = false;
static char g_response_body[1024];
static int g_call_count = 0;
static ChatEngineConfig* g_last_engine = NULL;
static char g_last_request_json[1024];

ChatProxyResult* mock_chat_proxy_send_request(const ChatEngineConfig* engine,
                                              const char* request_json,
                                              const ChatProxyConfig* config) {
    (void)config;

    g_call_count++;
    g_last_engine = (ChatEngineConfig*)engine;

    if (request_json) {
        strncpy(g_last_request_json, request_json, sizeof(g_last_request_json) - 1);
        g_last_request_json[sizeof(g_last_request_json) - 1] = '\0';
    } else {
        g_last_request_json[0] = '\0';
    }

    if (g_return_null) {
        return NULL;
    }

    ChatProxyResult* result = (ChatProxyResult*)malloc(sizeof(ChatProxyResult));
    if (!result) {
        return NULL;
    }

    result->code = CHAT_PROXY_OK;
    result->http_status = g_http_status;
    result->error_message = NULL;
    result->response_size = strlen(g_response_body);

    if (g_response_body[0]) {
        result->response_body = strdup(g_response_body);
    } else {
        result->response_body = NULL;
    }

    result->total_time_ms = 0.0;

    return result;
}

void mock_chat_proxy_result_destroy(ChatProxyResult* result) {
    if (!result) return;
    if (result->response_body) {
        free(result->response_body);
    }
    if (result->error_message) {
        free(result->error_message);
    }
    free(result);
}

void mock_chat_proxy_set_http_status(int http_status) {
    g_http_status = http_status;
}

void mock_chat_proxy_set_result_null(bool is_null) {
    g_return_null = is_null;
}

void mock_chat_proxy_set_response_body(const char* body) {
    if (body) {
        strncpy(g_response_body, body, sizeof(g_response_body) - 1);
        g_response_body[sizeof(g_response_body) - 1] = '\0';
    } else {
        g_response_body[0] = '\0';
    }
}

int mock_chat_proxy_call_count(void) {
    return g_call_count;
}

const ChatEngineConfig* mock_chat_proxy_last_engine(void) {
    return g_last_engine;
}

const char* mock_chat_proxy_last_request_json(void) {
    return g_last_request_json;
}

void mock_chat_proxy_reset_all(void) {
    g_http_status = 200;
    g_return_null = false;
    g_response_body[0] = '\0';
    g_call_count = 0;
    g_last_engine = NULL;
    g_last_request_json[0] = '\0';
}
