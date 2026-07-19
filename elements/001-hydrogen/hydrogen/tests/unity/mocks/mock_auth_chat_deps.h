/*
 * Mock chat dependencies for auth_chat success-path unit testing.
 *
 * Provides fake implementations of the network/storage functions exercised by
 * handle_auth_chat_request()'s success path so the full happy path can be
 * driven in a Unity test without a live AI provider or database:
 *   - chat_proxy_send_with_retry (libcurl HTTP call)
 *   - chat_storage_store_segment / chat_storage_segment_exists /
 *     chat_storage_cache_get_stats / chat_storage_store_chat /
 *     chat_storage_free_hash / chat_storage_resolve_media_in_content
 *
 * Enable with USE_MOCK_AUTH_CHAT_DEPS. Function names are mapped to the real
 * names via macros so call sites in auth_chat.c are unchanged.
 */

#ifndef MOCK_AUTH_CHAT_DEPS_H
#define MOCK_AUTH_CHAT_DEPS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <src/hydrogen.h>
#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/proxy.h>
#include <src/api/wschat/helpers/storage.h>

#ifdef USE_MOCK_AUTH_CHAT_DEPS

#define chat_proxy_send_with_retry mock_chat_proxy_send_with_retry
#define chat_proxy_send_multi mock_chat_proxy_send_multi
#define chat_storage_store_segment mock_chat_storage_store_segment
#define chat_storage_segment_exists mock_chat_storage_segment_exists
#define chat_storage_cache_get_stats mock_chat_storage_cache_get_stats
#define chat_storage_store_chat mock_chat_storage_store_chat
#define chat_storage_free_hash mock_chat_storage_free_hash
#define chat_storage_resolve_media_in_content mock_chat_storage_resolve_media_in_content

#endif // USE_MOCK_AUTH_CHAT_DEPS

// Mock implementations
ChatProxyResult* mock_chat_proxy_send_with_retry(const ChatEngineConfig* engine,
                                                  const char* request_json,
                                                  const ChatProxyConfig* config);
ChatMultiResult* mock_chat_proxy_send_multi(const ChatMultiRequest* requests,
                                             size_t request_count,
                                             const ChatProxyConfig* config);
char* mock_chat_storage_store_segment(const char* database, const char* message, size_t message_len);
bool mock_chat_storage_segment_exists(const char* database, const char* segment_hash);
bool mock_chat_storage_cache_get_stats(const char* database,
                                        uint64_t* hits, uint64_t* misses, double* hit_ratio);
char* mock_chat_storage_store_chat(const char* database, const char* convos_ref,
                                    const char** segment_hashes, size_t hash_count,
                                    const char* engine_name, const char* model,
                                    int tokens_prompt, int tokens_completion,
                                    double cost_total, int user_id, int duration_ms,
                                    const char* session_id);
void mock_chat_storage_free_hash(char* hash);
bool mock_chat_storage_resolve_media_in_content(const char* database,
                                                const char* content_json,
                                                char** resolved_json,
                                                char** error_message);

// Control functions
void mock_auth_chat_deps_set_proxy_success(bool success);
void mock_auth_chat_deps_set_proxy_response_body(const char* body);
void mock_auth_chat_deps_set_multi_failure_index(int failure_index);
void mock_auth_chat_deps_set_segment_exists(bool exists);
void mock_auth_chat_deps_set_cache_stats(uint64_t hits, uint64_t misses, double hit_ratio);
void mock_auth_chat_deps_set_media_resolve(bool success, const char* resolved_json, const char* error_message);
void mock_auth_chat_deps_reset_all(void);
int mock_auth_chat_deps_proxy_call_count(void);
int mock_auth_chat_deps_store_segment_count(void);

#endif // MOCK_AUTH_CHAT_DEPS_H
