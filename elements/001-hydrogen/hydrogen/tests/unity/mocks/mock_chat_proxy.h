/*
 * Mock chat proxy for wschat health unit testing.
 *
 * Provides a fake implementation of chat_proxy_send_request() and
 * chat_proxy_result_destroy() so the chat health-check functions can be
 * exercised in a Unity test without performing real network calls.
 *
 * Enable with USE_MOCK_CHAT_PROXY. The real function names are mapped to the
 * mock names via macros so call sites in health.c are unchanged.
 */

#ifndef MOCK_CHAT_PROXY_H
#define MOCK_CHAT_PROXY_H

#include <stddef.h>
#include <stdbool.h>

#include <src/hydrogen.h>
#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/proxy.h>

#ifdef USE_MOCK_CHAT_PROXY

#define chat_proxy_send_request mock_chat_proxy_send_request
#define chat_proxy_result_destroy mock_chat_proxy_result_destroy

#endif // USE_MOCK_CHAT_PROXY

// Mock implementations
ChatProxyResult* mock_chat_proxy_send_request(const ChatEngineConfig* engine,
                                             const char* request_json,
                                             const ChatProxyConfig* config);
void mock_chat_proxy_result_destroy(ChatProxyResult* result);

// Control functions
void mock_chat_proxy_set_http_status(int http_status);
void mock_chat_proxy_set_result_null(bool is_null);
void mock_chat_proxy_set_response_body(const char* body);
int mock_chat_proxy_call_count(void);
const ChatEngineConfig* mock_chat_proxy_last_engine(void);
const char* mock_chat_proxy_last_request_json(void);
void mock_chat_proxy_reset_all(void);

#endif // MOCK_CHAT_PROXY_H
