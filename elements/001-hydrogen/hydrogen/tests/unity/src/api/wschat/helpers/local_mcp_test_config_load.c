#include <src/hydrogen.h>
#include <unity.h>
#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/local_mcp.h>

void test_config_load_missing(void);
void test_config_load_enabled_servers(void);
void test_config_load_empty_allowlist_skipped(void);
void test_config_cleanup_null(void);

void setUp(void) {}
void tearDown(void) {}

void test_config_load_missing(void) {
    json_t *collection = json_loads("{\"name\":\"grok\"}", 0, NULL);
    ChatLocalMcpConfig cfg;
    TEST_ASSERT_TRUE(chat_local_mcp_config_load(collection, &cfg));
    TEST_ASSERT_FALSE(cfg.enabled);
    TEST_ASSERT_EQUAL_UINT(0, cfg.server_count);
    json_decref(collection);
}

void test_config_load_enabled_servers(void) {
    json_t *collection = json_loads(
        "{\"local_mcp\":{\"enabled\":true,\"servers\":["
        "{\"url\":\"https://mcp.example.com/mcp\",\"authorization\":\"Bearer x\","
        "\"allowed_tools\":[\"System.Info\"]}]}}", 0, NULL);
    ChatLocalMcpConfig cfg;
    TEST_ASSERT_TRUE(chat_local_mcp_config_load(collection, &cfg));
    TEST_ASSERT_TRUE(cfg.enabled);
    TEST_ASSERT_EQUAL_UINT(1, cfg.server_count);
    TEST_ASSERT_EQUAL_STRING("https://mcp.example.com/mcp", cfg.servers[0].url);
    TEST_ASSERT_EQUAL_STRING("Bearer x", cfg.servers[0].authorization);
    TEST_ASSERT_EQUAL_UINT(1, cfg.servers[0].allowed_tool_count);
    TEST_ASSERT_EQUAL_STRING("System.Info", cfg.servers[0].allowed_tools[0]);
    chat_local_mcp_config_cleanup(&cfg);
    json_decref(collection);
}

void test_config_load_empty_allowlist_skipped(void) {
    json_t *collection = json_loads(
        "{\"local_mcp\":{\"enabled\":true,\"servers\":["
        "{\"url\":\"https://mcp.example.com/mcp\",\"allowed_tools\":[]}]}}", 0, NULL);
    ChatLocalMcpConfig cfg;
    TEST_ASSERT_TRUE(chat_local_mcp_config_load(collection, &cfg));
    TEST_ASSERT_EQUAL_UINT(0, cfg.server_count);
    chat_local_mcp_config_cleanup(&cfg);
    json_decref(collection);
}

void test_config_cleanup_null(void) {
    chat_local_mcp_config_cleanup(NULL);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_config_load_missing);
    RUN_TEST(test_config_load_enabled_servers);
    RUN_TEST(test_config_load_empty_allowlist_skipped);
    RUN_TEST(test_config_cleanup_null);
    return UNITY_END();
}
