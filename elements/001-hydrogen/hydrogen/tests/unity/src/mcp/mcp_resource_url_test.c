/*
 * Unity Test File: mcp_resource_url
 * Phase 8b — fail-closed URL reachability check.
 */
#include <src/hydrogen.h>
#include <unity.h>
#include <src/mcp/mcp_resource_url.h>

void test_url_rejects_null(void);
void test_url_rejects_empty(void);
void test_url_rejects_no_scheme(void);
void test_url_rejects_http(void);
void test_url_rejects_ftp(void);
void test_url_rejects_localhost(void);
void test_url_rejects_127(void);
void test_url_rejects_rfc1918_10(void);
void test_url_rejects_rfc1918_172(void);
void test_url_rejects_rfc1918_192(void);
void test_url_rejects_link_local(void);
void test_url_rejects_zero(void);
void test_url_rejects_cgnat(void);
void test_url_rejects_ipv6_loopback(void);
void test_url_rejects_ipv6_link_local(void);
void test_url_rejects_ipv6_ula(void);
void test_url_rejects_empty_host(void);
void test_url_rejects_missing_host_after_scheme(void);
void test_url_accepts_https_with_path(void);
void test_url_accepts_https_with_port(void);
void test_url_accepts_https_external(void);

void test_url_rejects_null(void) {
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable(NULL));
}

void test_url_rejects_empty(void) {
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable(""));
}

void test_url_rejects_no_scheme(void) {
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("example.com/mcp"));
}

void test_url_rejects_http(void) {
    /* xAI Remote MCP requires https for hosted connector. */
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("http://example.com/mcp"));
}

void test_url_rejects_ftp(void) {
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("ftp://example.com/mcp"));
}

void test_url_rejects_localhost(void) {
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("https://localhost/mcp"));
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("https://LOCALHOST:443/mcp"));
}

void test_url_rejects_127(void) {
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("https://127.0.0.1/mcp"));
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("https://127.255.255.254:3100/mcp"));
}

void test_url_rejects_rfc1918_10(void) {
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("https://10.0.0.1/mcp"));
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("https://10.255.255.255/mcp"));
}

void test_url_rejects_rfc1918_172(void) {
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("https://172.16.0.1/mcp"));
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("https://172.31.255.255/mcp"));
    TEST_ASSERT_TRUE(mcp_mcp_resource_url_is_reachable("https://172.32.0.1/mcp"));
}

void test_url_rejects_rfc1918_192(void) {
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("https://192.168.0.1/mcp"));
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("https://192.168.255.255/mcp"));
}

void test_url_rejects_link_local(void) {
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("https://169.254.169.254/latest/meta-data"));
}

void test_url_rejects_zero(void) {
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("https://0.0.0.0/mcp"));
}

void test_url_rejects_cgnat(void) {
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("https://100.64.0.1/mcp"));
    TEST_ASSERT_TRUE(mcp_mcp_resource_url_is_reachable("https://100.128.0.1/mcp"));
}

void test_url_rejects_ipv6_loopback(void) {
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("https://[::1]/mcp"));
}

void test_url_rejects_ipv6_link_local(void) {
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("https://[fe80::1]/mcp"));
}

void test_url_rejects_ipv6_ula(void) {
    /* fc00::/7 — unique local addresses */
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("https://[fc00::1]/mcp"));
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("https://[fd00::1]/mcp"));
}

void test_url_rejects_empty_host(void) {
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("https:///mcp"));
}

void test_url_rejects_missing_host_after_scheme(void) {
    TEST_ASSERT_FALSE(mcp_mcp_resource_url_is_reachable("https://"));
}

void test_url_accepts_https_with_path(void) {
    TEST_ASSERT_TRUE(mcp_mcp_resource_url_is_reachable("https://hydrogen.example.com/mcp"));
}

void test_url_accepts_https_with_port(void) {
    TEST_ASSERT_TRUE(mcp_mcp_resource_url_is_reachable("https://hydrogen.example.com:8443/mcp"));
}

void test_url_accepts_https_external(void) {
    TEST_ASSERT_TRUE(mcp_mcp_resource_url_is_reachable("https://api.openai.com/mcp"));
}

void setUp(void) {}
void tearDown(void) {}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_url_rejects_null);
    RUN_TEST(test_url_rejects_empty);
    RUN_TEST(test_url_rejects_no_scheme);
    RUN_TEST(test_url_rejects_http);
    RUN_TEST(test_url_rejects_ftp);
    RUN_TEST(test_url_rejects_localhost);
    RUN_TEST(test_url_rejects_127);
    RUN_TEST(test_url_rejects_rfc1918_10);
    RUN_TEST(test_url_rejects_rfc1918_172);
    RUN_TEST(test_url_rejects_rfc1918_192);
    RUN_TEST(test_url_rejects_link_local);
    RUN_TEST(test_url_rejects_zero);
    RUN_TEST(test_url_rejects_cgnat);
    RUN_TEST(test_url_rejects_ipv6_loopback);
    RUN_TEST(test_url_rejects_ipv6_link_local);
    RUN_TEST(test_url_rejects_ipv6_ula);
    RUN_TEST(test_url_rejects_empty_host);
    RUN_TEST(test_url_rejects_missing_host_after_scheme);
    RUN_TEST(test_url_accepts_https_with_path);
    RUN_TEST(test_url_accepts_https_with_port);
    RUN_TEST(test_url_accepts_https_external);
    return UNITY_END();
}
