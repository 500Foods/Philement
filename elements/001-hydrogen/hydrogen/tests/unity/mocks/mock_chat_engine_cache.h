/*
 * Mock chat engine cache functions for unit testing
 *
 * This file provides mock implementations for chat engine cache functions
 * needed by the LLM scripting API tests. The functions override the real
 * implementations when USE_MOCK_CHAT_ENGINE_CACHE is defined.
 */

#ifndef MOCK_CHAT_ENGINE_CACHE_H
#define MOCK_CHAT_ENGINE_CACHE_H

#include <src/api/wschat/helpers/engine_cache.h>

#ifdef USE_MOCK_CHAT_ENGINE_CACHE

#define chat_engine_cache_get_all mock_chat_engine_cache_get_all
#define chat_engine_provider_to_string mock_chat_engine_provider_to_string
#define chat_engine_cache_lookup_by_name mock_chat_engine_cache_lookup_by_name

#endif

ChatEngineConfig** mock_chat_engine_cache_get_all(ChatEngineCache* cache, size_t* count);
const char* mock_chat_engine_provider_to_string(ChatEngineProvider provider);
ChatEngineConfig* mock_chat_engine_cache_lookup_by_name(ChatEngineCache* cache, const char* name);

void mock_chat_engine_cache_set_get_all_result(ChatEngineConfig** result, size_t count);
void mock_chat_engine_cache_set_lookup_by_name_result(ChatEngineConfig* result);
void mock_chat_engine_cache_reset_all(void);

#endif
