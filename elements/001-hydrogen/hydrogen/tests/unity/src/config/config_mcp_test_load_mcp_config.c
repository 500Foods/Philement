/*
 * Unity Test File: load_mcp_config
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/config/config_mcp.h>
#include <src/config/config.h>

bool load_mcp_config(json_t *root, AppConfig *config);
void cleanup_mcp_config(MCPConfig *config);
void dump_mcp_config(const MCPConfig *config);
bool initialize_config_defaults(AppConfig *config);

void test_load_mcp_config_null_config(void);
void test_load_mcp_config_null_root(void);
void test_load_mcp_config_missing_section(void);
void test_load_mcp_config_full_custom(void);
void test_load_mcp_config_invalid_port(void);
void test_load_mcp_config_interface_any(void);
void test_cleanup_mcp_config_null(void);
void test_dump_mcp_config_null(void);
void test_dump_mcp_config_smoke(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_load_mcp_config_null_config(void) {
    json_t *root = json_object();

    TEST_ASSERT_FALSE(load_mcp_config(root, NULL));

    json_decref(root);
}

void test_load_mcp_config_null_root(void) {
    AppConfig config = {0};

    TEST_ASSERT_TRUE(initialize_config_defaults(&config));
    TEST_ASSERT_TRUE(load_mcp_config(NULL, &config));
    TEST_ASSERT_FALSE(config.mcp.Enabled);
    TEST_ASSERT_EQUAL_STRING("127.0.0.1", config.mcp.Interface);
    TEST_ASSERT_EQUAL(3100, config.mcp.Port);
    TEST_ASSERT_EQUAL_STRING("/mcp", config.mcp.Path);
    TEST_ASSERT_NULL(config.mcp.Protocol);
    TEST_ASSERT_TRUE(config.mcp.RequireJWT);
    TEST_ASSERT_TRUE(config.mcp.AcceptHydrogenJWT);
    TEST_ASSERT_FALSE(config.mcp.AcceptOidcIdP);
    TEST_ASSERT_FALSE(config.mcp.AcceptOidcRp);
    TEST_ASSERT_EQUAL(4, config.mcp.ThreadPoolSize);
    TEST_ASSERT_EQUAL(1048576, config.mcp.MaxBodyBytes);
    TEST_ASSERT_EQUAL(262144, config.mcp.MaxResultBytes);
    TEST_ASSERT_EQUAL(0, config.mcp.AllowedOriginCount);

    cleanup_mcp_config(&config.mcp);
}

void test_load_mcp_config_missing_section(void) {
    AppConfig config = {0};
    json_t *root = json_object();

    initialize_config_defaults(&config);
    TEST_ASSERT_TRUE(load_mcp_config(root, &config));
    TEST_ASSERT_FALSE(config.mcp.Enabled);
    TEST_ASSERT_EQUAL(3100, config.mcp.Port);

    json_decref(root);
    cleanup_mcp_config(&config.mcp);
}

void test_load_mcp_config_full_custom(void) {
    AppConfig config = {0};
    json_t *root = json_object();
    json_t *mcp = json_object();
    json_t *origins = json_array();
    json_t *scopes = json_array();

    initialize_config_defaults(&config);

    json_object_set_new(mcp, "Enabled", json_true());
    json_object_set_new(mcp, "Interface", json_string("127.0.0.1"));
    json_object_set_new(mcp, "Port", json_integer(3101));
    json_object_set_new(mcp, "Path", json_string("/mcp"));
    json_object_set_new(mcp, "Protocol", json_string("Mcp.Server"));
    json_object_set_new(mcp, "RequireJWT", json_true());
    json_object_set_new(mcp, "AcceptHydrogenJWT", json_true());
    json_object_set_new(mcp, "AcceptOidcIdP", json_true());
    json_object_set_new(mcp, "AcceptOidcRp", json_false());
    json_object_set_new(mcp, "Resource", json_string("http://127.0.0.1:3101/mcp"));
    json_object_set_new(mcp, "Database", json_string("Acuranzo"));
    json_object_set_new(mcp, "MaxBodyBytes", json_integer(2048));
    json_object_set_new(mcp, "MaxResultBytes", json_integer(4096));
    json_object_set_new(mcp, "RequestTimeoutSeconds", json_integer(15));
    json_object_set_new(mcp, "ThreadPoolSize", json_integer(8));
    json_object_set_new(mcp, "SessionIdleTimeoutSeconds", json_integer(60));
    json_object_set_new(mcp, "MaxSessions", json_integer(16));
    json_array_append_new(origins, json_string("http://localhost:3000"));
    json_object_set_new(mcp, "AllowedOrigins", origins);
    json_array_append_new(scopes, json_string("mcp"));
    json_object_set_new(mcp, "RequiredScopes", scopes);
    json_object_set_new(root, "MCP", mcp);

    TEST_ASSERT_TRUE(load_mcp_config(root, &config));
    TEST_ASSERT_TRUE(config.mcp.Enabled);
    TEST_ASSERT_EQUAL(3101, config.mcp.Port);
    TEST_ASSERT_EQUAL_STRING("Mcp.Server", config.mcp.Protocol);
    TEST_ASSERT_TRUE(config.mcp.AcceptOidcIdP);
    TEST_ASSERT_EQUAL(8, config.mcp.ThreadPoolSize);
    TEST_ASSERT_EQUAL(1, config.mcp.AllowedOriginCount);
    TEST_ASSERT_EQUAL_STRING("http://localhost:3000", config.mcp.AllowedOrigins[0]);
    TEST_ASSERT_EQUAL(1, config.mcp.RequiredScopeCount);
    TEST_ASSERT_EQUAL_STRING("mcp", config.mcp.RequiredScopes[0]);
    TEST_ASSERT_EQUAL(16, config.mcp.MaxSessions);

    json_decref(root);
    cleanup_mcp_config(&config.mcp);
}

void test_load_mcp_config_invalid_port(void) {
    AppConfig config = {0};
    json_t *root = json_object();
    json_t *mcp = json_object();

    initialize_config_defaults(&config);
    json_object_set_new(mcp, "Port", json_integer(70000));
    json_object_set_new(root, "MCP", mcp);

    TEST_ASSERT_FALSE(load_mcp_config(root, &config));

    json_decref(root);
    cleanup_mcp_config(&config.mcp);
}

void test_load_mcp_config_interface_any(void) {
    AppConfig config = {0};
    json_t *root = json_object();
    json_t *mcp = json_object();

    initialize_config_defaults(&config);
    json_object_set_new(mcp, "Interface", json_string("0.0.0.0"));
    json_object_set_new(root, "MCP", mcp);

    TEST_ASSERT_TRUE(load_mcp_config(root, &config));
    TEST_ASSERT_EQUAL_STRING("0.0.0.0", config.mcp.Interface);

    json_decref(root);
    cleanup_mcp_config(&config.mcp);
}

void test_cleanup_mcp_config_null(void) {
    cleanup_mcp_config(NULL);
}

void test_dump_mcp_config_null(void) {
    dump_mcp_config(NULL);
}

void test_dump_mcp_config_smoke(void) {
    AppConfig config = {0};

    initialize_config_defaults(&config);
    dump_mcp_config(&config.mcp);
    cleanup_mcp_config(&config.mcp);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_load_mcp_config_null_config);
    RUN_TEST(test_load_mcp_config_null_root);
    RUN_TEST(test_load_mcp_config_missing_section);
    RUN_TEST(test_load_mcp_config_full_custom);
    RUN_TEST(test_load_mcp_config_invalid_port);
    RUN_TEST(test_load_mcp_config_interface_any);
    RUN_TEST(test_cleanup_mcp_config_null);
    RUN_TEST(test_dump_mcp_config_null);
    RUN_TEST(test_dump_mcp_config_smoke);
    return UNITY_END();
}
