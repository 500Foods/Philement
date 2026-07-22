/*
 * Mock chat engine cache functions for unit testing
 *
 * This file provides mock implementations for chat engine cache functions
 * needed by the LLM scripting API tests.
 */

#include "mock_chat_engine_cache.h"
#include <stdlib.h>

static ChatEngineConfig** mock_chat_engine_cache_get_all_result = NULL;
static size_t mock_chat_engine_cache_get_all_count = 0;
static ChatEngineConfig* mock_chat_engine_cache_lookup_by_name_result = NULL;

ChatEngineConfig** mock_chat_engine_cache_get_all(ChatEngineCache* cache, size_t* count) {
    (void)cache;

    if (count) {
        *count = mock_chat_engine_cache_get_all_count;
    }

    return mock_chat_engine_cache_get_all_result;
}

const char* mock_chat_engine_provider_to_string(ChatEngineProvider provider) {
    (void)provider;
    return "openai";
}

ChatEngineConfig* mock_chat_engine_cache_lookup_by_name(ChatEngineCache* cache, const char* name) {
    (void)cache;
    (void)name;
    return mock_chat_engine_cache_lookup_by_name_result;
}

void mock_chat_engine_cache_set_get_all_result(ChatEngineConfig** result, size_t count) {
    mock_chat_engine_cache_get_all_result = result;
    mock_chat_engine_cache_get_all_count = count;
}

void mock_chat_engine_cache_set_lookup_by_name_result(ChatEngineConfig* result) {
    mock_chat_engine_cache_lookup_by_name_result = result;
}

void mock_chat_engine_cache_reset_all(void) {
    mock_chat_engine_cache_get_all_result = NULL;
    mock_chat_engine_cache_get_all_count = 0;
    mock_chat_engine_cache_lookup_by_name_result = NULL;
}
