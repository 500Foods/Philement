/*
 * Unity Tests for Chat Engine Cache (CEC)
 *
 * Tests the chat engine cache functionality for storing and retrieving AI engine configurations.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/conduit/chat_common/chat_engine_cache.h>

// Test function prototypes
void test_chat_engine_cache_create_destroy(void);
void test_chat_engine_config_create(void);
void test_chat_engine_cache_add_lookup(void);
void test_chat_engine_cache_lookup_by_name(void);
void test_chat_engine_cache_get_default(void);
void test_chat_engine_cache_get_all(void);
void test_chat_engine_cache_clear(void);
void test_chat_engine_provider_conversion(void);
void test_chat_engine_cache_stats(void);
void test_chat_engine_cache_refresh_check(void);

// Test fixtures
static ChatEngineCache* test_cache = NULL;

void setUp(void) {
    test_cache = chat_engine_cache_create("test_db");
    TEST_ASSERT_NOT_NULL(test_cache);
}

void tearDown(void) {
    if (test_cache) {
        chat_engine_cache_destroy(test_cache);
        test_cache = NULL;
    }
}

// Test cache creation and destruction
void test_chat_engine_cache_create_destroy(void) {
    ChatEngineCache* cache = chat_engine_cache_create("test_db");
    TEST_ASSERT_NOT_NULL(cache);
    TEST_ASSERT_EQUAL(0, chat_engine_cache_get_engine_count(cache));

    chat_engine_cache_destroy(cache);
}

// Test engine config creation
void test_chat_engine_config_create(void) {
    ChatEngineConfig* engine = chat_engine_config_create(
        1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4-turbo",
        "https://api.openai.com/v1/chat/completions", "sk-test123",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);

    TEST_ASSERT_NOT_NULL(engine);
    TEST_ASSERT_EQUAL(1, engine->engine_id);
    TEST_ASSERT_EQUAL_STRING("gpt4", engine->name);
    TEST_ASSERT_EQUAL(CEC_PROVIDER_OPENAI, engine->provider);
    TEST_ASSERT_EQUAL_STRING("gpt-4-turbo", engine->model);
    TEST_ASSERT_EQUAL_STRING("https://api.openai.com/v1/chat/completions", engine->api_url);
    TEST_ASSERT_EQUAL(4096, engine->max_tokens);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.7, engine->temperature_default);
    TEST_ASSERT_TRUE(engine->is_default);
    TEST_ASSERT_EQUAL(0, engine->usage_count);
    TEST_ASSERT_FALSE(engine->use_native_api);

    chat_engine_config_destroy(engine);
}

// Test adding and looking up engines
void test_chat_engine_cache_add_lookup(void) {
    ChatEngineConfig* engine = chat_engine_config_create(
        1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4-turbo",
        "https://api.openai.com/v1/chat/completions", "sk-test123",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);

    TEST_ASSERT_TRUE(chat_engine_cache_add_engine(test_cache, engine));
    TEST_ASSERT_EQUAL(1, chat_engine_cache_get_engine_count(test_cache));

    // Verify lookup by ID
    ChatEngineConfig* found = chat_engine_cache_lookup_by_id(test_cache, 1);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL(1, found->engine_id);
    TEST_ASSERT_EQUAL_STRING("gpt4", found->name);

    // Verify usage count was updated
    TEST_ASSERT_EQUAL(1, found->usage_count);
}

// Test lookup by name
void test_chat_engine_cache_lookup_by_name(void) {
    ChatEngineConfig* engine1 = chat_engine_config_create(
        1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4-turbo",
        "https://api.openai.com/v1/chat/completions", "sk-test123",
        4096, 0.7, false, 300, 10, 10, 100, MODALITY_DEFAULT, false);

    ChatEngineConfig* engine2 = chat_engine_config_create(
        2, "claude", CEC_PROVIDER_ANTHROPIC, "claude-3-opus",
        "https://api.anthropic.com/v1/messages", "sk-ant-test",
        8192, 0.5, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);

    chat_engine_cache_add_engine(test_cache, engine1);
    chat_engine_cache_add_engine(test_cache, engine2);
    TEST_ASSERT_EQUAL(2, chat_engine_cache_get_engine_count(test_cache));

    // Lookup by name
    ChatEngineConfig* found = chat_engine_cache_lookup_by_name(test_cache, "claude");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL(2, found->engine_id);
    TEST_ASSERT_EQUAL(CEC_PROVIDER_ANTHROPIC, found->provider);

    // Lookup non-existent
    ChatEngineConfig* not_found = chat_engine_cache_lookup_by_name(test_cache, "nonexistent");
    TEST_ASSERT_NULL(not_found);
}

// Test getting default engine
void test_chat_engine_cache_get_default(void) {
    ChatEngineConfig* engine1 = chat_engine_config_create(
        1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4-turbo",
        "https://api.openai.com/v1/chat/completions", "sk-test123",
        4096, 0.7, false, 300, 10, 10, 100, MODALITY_DEFAULT, false);

    ChatEngineConfig* engine2 = chat_engine_config_create(
        2, "claude", CEC_PROVIDER_ANTHROPIC, "claude-3-opus",
        "https://api.anthropic.com/v1/messages", "sk-ant-test",
        8192, 0.5, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);

    chat_engine_cache_add_engine(test_cache, engine1);
    chat_engine_cache_add_engine(test_cache, engine2);

    // Get default should return claude (marked as default)
    ChatEngineConfig* default_engine = chat_engine_cache_get_default(test_cache);
    TEST_ASSERT_NOT_NULL(default_engine);
    TEST_ASSERT_EQUAL_STRING("claude", default_engine->name);

    // Usage count should be updated
    TEST_ASSERT_EQUAL(1, default_engine->usage_count);
}

// Test getting all engines
void test_chat_engine_cache_get_all(void) {
    ChatEngineConfig* engine1 = chat_engine_config_create(
        1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4-turbo",
        "https://api.openai.com/v1/chat/completions", "sk-test123",
        4096, 0.7, false, 300, 10, 10, 100, MODALITY_DEFAULT, false);

    ChatEngineConfig* engine2 = chat_engine_config_create(
        2, "claude", CEC_PROVIDER_ANTHROPIC, "claude-3-opus",
        "https://api.anthropic.com/v1/messages", "sk-ant-test",
        8192, 0.5, false, 300, 10, 10, 100, MODALITY_DEFAULT, false);

    chat_engine_cache_add_engine(test_cache, engine1);
    chat_engine_cache_add_engine(test_cache, engine2);

    size_t count = 0;
    ChatEngineConfig** engines = chat_engine_cache_get_all(test_cache, &count);
    TEST_ASSERT_NOT_NULL(engines);
    TEST_ASSERT_EQUAL(2, count);

    // Free the array (but not entries - they're still owned by cache)
    free(engines);
}

// Test clearing cache
void test_chat_engine_cache_clear(void) {
    ChatEngineConfig* engine = chat_engine_config_create(
        1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4-turbo",
        "https://api.openai.com/v1/chat/completions", "sk-test123",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);

    chat_engine_cache_add_engine(test_cache, engine);
    TEST_ASSERT_EQUAL(1, chat_engine_cache_get_engine_count(test_cache));

    chat_engine_cache_clear(test_cache);
    TEST_ASSERT_EQUAL(0, chat_engine_cache_get_engine_count(test_cache));

    // Should not find engine after clear
    ChatEngineConfig* found = chat_engine_cache_lookup_by_id(test_cache, 1);
    TEST_ASSERT_NULL(found);
}

// Test provider string conversion
void test_chat_engine_provider_conversion(void) {
    // String to enum
    TEST_ASSERT_EQUAL(CEC_PROVIDER_OPENAI, chat_engine_provider_from_string("openai"));
    TEST_ASSERT_EQUAL(CEC_PROVIDER_OPENAI, chat_engine_provider_from_string("xai"));
    TEST_ASSERT_EQUAL(CEC_PROVIDER_OPENAI, chat_engine_provider_from_string("gradient"));
    TEST_ASSERT_EQUAL(CEC_PROVIDER_ANTHROPIC, chat_engine_provider_from_string("anthropic"));
    TEST_ASSERT_EQUAL(CEC_PROVIDER_OLLAMA, chat_engine_provider_from_string("ollama"));
    TEST_ASSERT_EQUAL(CEC_PROVIDER_UNKNOWN, chat_engine_provider_from_string("unknown"));
    TEST_ASSERT_EQUAL(CEC_PROVIDER_UNKNOWN, chat_engine_provider_from_string(NULL));

    // Enum to string
    TEST_ASSERT_EQUAL_STRING("openai", chat_engine_provider_to_string(CEC_PROVIDER_OPENAI));
    TEST_ASSERT_EQUAL_STRING("anthropic", chat_engine_provider_to_string(CEC_PROVIDER_ANTHROPIC));
    TEST_ASSERT_EQUAL_STRING("ollama", chat_engine_provider_to_string(CEC_PROVIDER_OLLAMA));
    TEST_ASSERT_EQUAL_STRING("unknown", chat_engine_provider_to_string(CEC_PROVIDER_UNKNOWN));
}

// Test cache statistics
void test_chat_engine_cache_stats(void) {
    ChatEngineConfig* engine1 = chat_engine_config_create(
        1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4-turbo",
        "https://api.openai.com/v1/chat/completions", "sk-test123",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);

    ChatEngineConfig* engine2 = chat_engine_config_create(
        2, "claude", CEC_PROVIDER_ANTHROPIC, "claude-3-opus",
        "https://api.anthropic.com/v1/messages", "sk-ant-test",
        8192, 0.5, false, 300, 10, 10, 100, MODALITY_DEFAULT, false);

    chat_engine_cache_add_engine(test_cache, engine1);
    chat_engine_cache_add_engine(test_cache, engine2);

    // Use engines to update stats
    chat_engine_cache_lookup_by_id(test_cache, 1);
    chat_engine_cache_lookup_by_id(test_cache, 2);
    chat_engine_cache_lookup_by_id(test_cache, 1);

    char buffer[256];
    chat_engine_cache_get_stats(test_cache, buffer, sizeof(buffer));

    // Buffer should contain engine count
    TEST_ASSERT_NOT_NULL(strstr(buffer, "2"));  // 2 engines
    TEST_ASSERT_NOT_NULL(strstr(buffer, "1"));  // 1 default
}

// Test refresh timing
void test_chat_engine_cache_refresh_check(void) {
    TEST_ASSERT_EQUAL(0, chat_engine_cache_get_last_refresh(test_cache));

    // Should need refresh (never refreshed)
    TEST_ASSERT_TRUE(chat_engine_cache_needs_refresh(test_cache, 3600));

    // Add an engine (this updates last_refresh if we were loading from DB)
    ChatEngineConfig* engine = chat_engine_config_create(
        1, "gpt4", CEC_PROVIDER_OPENAI, "gpt-4-turbo",
        "https://api.openai.com/v1/chat/completions", "sk-test123",
        4096, 0.7, true, 300, 10, 10, 100, MODALITY_DEFAULT, false);
    chat_engine_cache_add_engine(test_cache, engine);

    // Simulate a recent refresh
    test_cache->last_refresh = time(NULL);
    TEST_ASSERT_FALSE(chat_engine_cache_needs_refresh(test_cache, 3600));

    // Simulate time passing (set refresh to 1 second ago)
    test_cache->last_refresh = time(NULL) - 2;
    TEST_ASSERT_TRUE(chat_engine_cache_needs_refresh(test_cache, 1));  // 1 second interval exceeded
}

// Main function
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_chat_engine_cache_create_destroy);
    RUN_TEST(test_chat_engine_config_create);
    RUN_TEST(test_chat_engine_cache_add_lookup);
    RUN_TEST(test_chat_engine_cache_lookup_by_name);
    RUN_TEST(test_chat_engine_cache_get_default);
    RUN_TEST(test_chat_engine_cache_get_all);
    RUN_TEST(test_chat_engine_cache_clear);
    RUN_TEST(test_chat_engine_provider_conversion);
    RUN_TEST(test_chat_engine_cache_stats);
    RUN_TEST(test_chat_engine_cache_refresh_check);

    return UNITY_END();
}
