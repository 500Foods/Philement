#include <src/hydrogen.h>
#include <unity.h>
#include <src/mcp/mcp_prm.h>
#include <src/mcp/mcp_auth.h>
#include <src/config/config_mcp.h>
#include <src/config/config.h>
#include <src/config/config_oidc.h>
#include <src/config/config_oidc_rp.h>

void test_prm_build_null_cfg_null_app(void);
void test_prm_build_empty_cfg(void);
void test_prm_build_accept_hydrogen_jwt(void);
void test_prm_build_accept_oidc_idp(void);
void test_prm_build_accept_oidc_rp(void);
void test_prm_build_resource_url(void);
void test_prm_build_no_servers(void);
void test_prm_build_idp_without_issuer(void);
void test_prm_build_rp_provider_no_issuer(void);

void setUp(void) {}
void tearDown(void) {}

static void assert_json_field(json_t *root, const char *key, const char *expected) {
    json_t *val = json_object_get(root, key);
    if (expected == NULL) {
        TEST_ASSERT_NULL(val);
        return;
    }
    TEST_ASSERT_NOT_NULL(val);
    TEST_ASSERT_TRUE(json_is_string(val));
    TEST_ASSERT_EQUAL_STRING(expected, json_string_value(val));
}

void test_prm_build_null_cfg_null_app(void) {
    char *out = mcp_prm_build(NULL, NULL);
    TEST_ASSERT_NOT_NULL(out);
    json_t *root = json_loads(out, 0, NULL);
    TEST_ASSERT_NOT_NULL(root);
    assert_json_field(root, "resource", "");
    json_t *algs = json_object_get(root, "resource_signing_alg_values_supported");
    TEST_ASSERT_NOT_NULL(algs);
    TEST_ASSERT_TRUE(json_is_array(algs));
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(algs));
    TEST_ASSERT_EQUAL_STRING("HS256", json_string_value(json_array_get(algs, 0)));
    json_t *servers = json_object_get(root, "authorization_servers");
    TEST_ASSERT_NOT_NULL(servers);
    TEST_ASSERT_TRUE(json_is_array(servers));
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(servers));
    json_t *methods = json_object_get(root, "bearer_methods_supported");
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(methods));
    TEST_ASSERT_EQUAL_STRING("header", json_string_value(json_array_get(methods, 0)));
    json_decref(root);
    free(out);
}

void test_prm_build_empty_cfg(void) {
    MCPConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    mcp_config_apply_defaults(&cfg);
    AppConfig app;
    memset(&app, 0, sizeof(app));

    char *out = mcp_prm_build(&cfg, &app);
    TEST_ASSERT_NOT_NULL(out);
    json_t *root = json_loads(out, 0, NULL);
    TEST_ASSERT_NOT_NULL(root);
    /* AcceptHydrogenJWT defaults to true, so HS256 should be present */
    json_t *algs = json_object_get(root, "resource_signing_alg_values_supported");
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(algs));
    TEST_ASSERT_EQUAL_STRING("HS256", json_string_value(json_array_get(algs, 0)));
    /* Resource is NULL by default, so derived from interface:port/path */
    assert_json_field(root, "resource", "http://127.0.0.1:3100/mcp");
    json_decref(root);
    free(out);
}

void test_prm_build_accept_hydrogen_jwt(void) {
    MCPConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    mcp_config_apply_defaults(&cfg);
    cfg.AcceptOidcIdP = false;
    cfg.AcceptOidcRp = false;

    char *out = mcp_prm_build(&cfg, NULL);
    TEST_ASSERT_NOT_NULL(out);
    json_t *root = json_loads(out, 0, NULL);
    TEST_ASSERT_NOT_NULL(root);
    json_t *algs = json_object_get(root, "resource_signing_alg_values_supported");
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(algs));
    TEST_ASSERT_EQUAL_STRING("HS256", json_string_value(json_array_get(algs, 0)));
    json_decref(root);
    free(out);
}

void test_prm_build_accept_oidc_idp(void) {
    MCPConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    mcp_config_apply_defaults(&cfg);
    cfg.AcceptHydrogenJWT = false;
    cfg.AcceptOidcIdP = true;

    AppConfig app;
    memset(&app, 0, sizeof(app));
    app.oidc.enabled = true;
    app.oidc.issuer = strdup("https://idp.example.com");

    char *out = mcp_prm_build(&cfg, &app);
    TEST_ASSERT_NOT_NULL(out);
    json_t *root = json_loads(out, 0, NULL);
    TEST_ASSERT_NOT_NULL(root);
    json_t *algs = json_object_get(root, "resource_signing_alg_values_supported");
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(algs));
    TEST_ASSERT_EQUAL_STRING("RS256", json_string_value(json_array_get(algs, 0)));
    json_t *servers = json_object_get(root, "authorization_servers");
    TEST_ASSERT_NOT_NULL(servers);
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(servers));
    TEST_ASSERT_EQUAL_STRING("https://idp.example.com", json_string_value(json_array_get(servers, 0)));
    json_decref(root);
    free(out);
    free(app.oidc.issuer);
}

void test_prm_build_accept_oidc_rp(void) {
    MCPConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    mcp_config_apply_defaults(&cfg);
    cfg.AcceptHydrogenJWT = false;
    cfg.AcceptOidcRp = true;

    AppConfig app;
    memset(&app, 0, sizeof(app));
    app.oidc_rp.provider_count = 2;
    app.oidc_rp.providers[0].issuer = strdup("https://rp1.example.com");
    app.oidc_rp.providers[1].issuer = strdup("https://rp2.example.com");

    char *out = mcp_prm_build(&cfg, &app);
    TEST_ASSERT_NOT_NULL(out);
    json_t *root = json_loads(out, 0, NULL);
    TEST_ASSERT_NOT_NULL(root);
    json_t *algs = json_object_get(root, "resource_signing_alg_values_supported");
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(algs));
    TEST_ASSERT_EQUAL_STRING("RS256", json_string_value(json_array_get(algs, 0)));
    json_t *servers = json_object_get(root, "authorization_servers");
    TEST_ASSERT_NOT_NULL(servers);
    TEST_ASSERT_EQUAL_UINT(2, json_array_size(servers));
    TEST_ASSERT_EQUAL_STRING("https://rp1.example.com", json_string_value(json_array_get(servers, 0)));
    TEST_ASSERT_EQUAL_STRING("https://rp2.example.com", json_string_value(json_array_get(servers, 1)));
    json_decref(root);
    free(out);
    free(app.oidc_rp.providers[0].issuer);
    free(app.oidc_rp.providers[1].issuer);
}

void test_prm_build_resource_url(void) {
    MCPConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    mcp_config_apply_defaults(&cfg);
    cfg.Resource = strdup("http://127.0.0.1:3100/mcp");

    char *out = mcp_prm_build(&cfg, NULL);
    TEST_ASSERT_NOT_NULL(out);
    json_t *root = json_loads(out, 0, NULL);
    TEST_ASSERT_NOT_NULL(root);
    assert_json_field(root, "resource", "http://127.0.0.1:3100/mcp");
    json_decref(root);
    free(out);
    free(cfg.Resource);
}

void test_prm_build_no_servers(void) {
    MCPConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    mcp_config_apply_defaults(&cfg);

    char *out = mcp_prm_build(&cfg, NULL);
    TEST_ASSERT_NOT_NULL(out);
    json_t *root = json_loads(out, 0, NULL);
    TEST_ASSERT_NOT_NULL(root);
    json_t *servers = json_object_get(root, "authorization_servers");
    TEST_ASSERT_NOT_NULL(servers);
    TEST_ASSERT_TRUE(json_is_array(servers));
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(servers));
    json_decref(root);
    free(out);
}

void test_prm_build_idp_without_issuer(void) {
    MCPConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    mcp_config_apply_defaults(&cfg);
    cfg.AcceptHydrogenJWT = false;
    cfg.AcceptOidcIdP = true;

    AppConfig app;
    memset(&app, 0, sizeof(app));
    app.oidc.enabled = true;
    app.oidc.issuer = NULL;

    char *out = mcp_prm_build(&cfg, &app);
    TEST_ASSERT_NOT_NULL(out);
    json_t *root = json_loads(out, 0, NULL);
    TEST_ASSERT_NOT_NULL(root);
    /* Should still get RS256 alg but no servers */
    json_t *algs = json_object_get(root, "resource_signing_alg_values_supported");
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(algs));
    TEST_ASSERT_EQUAL_STRING("RS256", json_string_value(json_array_get(algs, 0)));
    json_t *servers = json_object_get(root, "authorization_servers");
    TEST_ASSERT_NOT_NULL(servers);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(servers));
    json_decref(root);
    free(out);
}

void test_prm_build_rp_provider_no_issuer(void) {
    MCPConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    mcp_config_apply_defaults(&cfg);
    cfg.AcceptHydrogenJWT = false;
    cfg.AcceptOidcRp = true;

    AppConfig app;
    memset(&app, 0, sizeof(app));
    app.oidc_rp.provider_count = 1;
    app.oidc_rp.providers[0].issuer = NULL;

    char *out = mcp_prm_build(&cfg, &app);
    TEST_ASSERT_NOT_NULL(out);
    json_t *root = json_loads(out, 0, NULL);
    TEST_ASSERT_NOT_NULL(root);
    /* RS256 present but no servers because issuer is NULL */
    json_t *algs = json_object_get(root, "resource_signing_alg_values_supported");
    TEST_ASSERT_EQUAL_UINT(1, json_array_size(algs));
    TEST_ASSERT_EQUAL_STRING("RS256", json_string_value(json_array_get(algs, 0)));
    json_t *servers = json_object_get(root, "authorization_servers");
    TEST_ASSERT_NOT_NULL(servers);
    TEST_ASSERT_EQUAL_UINT(0, json_array_size(servers));
    json_decref(root);
    free(out);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_prm_build_null_cfg_null_app);
    RUN_TEST(test_prm_build_empty_cfg);
    RUN_TEST(test_prm_build_accept_hydrogen_jwt);
    RUN_TEST(test_prm_build_accept_oidc_idp);
    RUN_TEST(test_prm_build_accept_oidc_rp);
    RUN_TEST(test_prm_build_resource_url);
    RUN_TEST(test_prm_build_no_servers);
    RUN_TEST(test_prm_build_idp_without_issuer);
    RUN_TEST(test_prm_build_rp_provider_no_issuer);
    return UNITY_END();
}
