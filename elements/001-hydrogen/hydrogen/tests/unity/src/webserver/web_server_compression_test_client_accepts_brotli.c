/*
 * Unity Test File: Web Server Compression - Client Accepts Brotli Test
 * This file contains unit tests for client_accepts_brotli() function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/webserver/web_server_compression.h>

// Mock MHD connection structure
struct MockMHDConnection {
    const char *accept_encoding;
};

// Global mock state
static struct MockMHDConnection mock_connection = {0};

// Mock MHD function
const char *MHD_lookup_connection_value(struct MHD_Connection *connection,
                                      enum MHD_ValueKind kind, const char *key) {
    (void)connection;
    (void)kind;
    if (strcmp(key, "Accept-Encoding") == 0) {
        return mock_connection.accept_encoding;
    }
    return NULL;
}

void setUp(void) {
    // Reset mock state
    memset(&mock_connection, 0, sizeof(mock_connection));
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
static void test_client_accepts_brotli_null_connection(void) {
    // Test null parameter handling
    TEST_ASSERT_FALSE(client_accepts_brotli(NULL));
}

static void test_client_accepts_brotli_no_accept_encoding(void) {
    // Test with no Accept-Encoding header
    mock_connection.accept_encoding = NULL;
    TEST_ASSERT_FALSE(client_accepts_brotli((struct MHD_Connection*)&mock_connection));
}

static void test_client_accepts_brotli_empty_accept_encoding(void) {
    // Test with empty Accept-Encoding header
    mock_connection.accept_encoding = "";
    TEST_ASSERT_FALSE(client_accepts_brotli((struct MHD_Connection*)&mock_connection));
}

static void test_client_accepts_brotli_brotli_not_supported(void) {
    // Test with Accept-Encoding that doesn't include brotli
    mock_connection.accept_encoding = "gzip, deflate";
    TEST_ASSERT_FALSE(client_accepts_brotli((struct MHD_Connection*)&mock_connection));
}

static void test_client_accepts_brotli_brotli_at_start(void) {
    // Test with brotli at the start of encoding list
    mock_connection.accept_encoding = "br, gzip, deflate";
    TEST_ASSERT_TRUE(client_accepts_brotli((struct MHD_Connection*)&mock_connection));
}

static void test_client_accepts_brotli_brotli_in_middle(void) {
    // Test with brotli in middle of encoding list
    mock_connection.accept_encoding = "gzip, br, deflate";
    TEST_ASSERT_TRUE(client_accepts_brotli((struct MHD_Connection*)&mock_connection));
}

static void test_client_accepts_brotli_brotli_at_end(void) {
    // Test with brotli at the end of encoding list
    mock_connection.accept_encoding = "gzip, deflate, br";
    TEST_ASSERT_TRUE(client_accepts_brotli((struct MHD_Connection*)&mock_connection));
}

static void test_client_accepts_brotli_brotli_only(void) {
    // Test with only brotli encoding
    mock_connection.accept_encoding = "br";
    TEST_ASSERT_TRUE(client_accepts_brotli((struct MHD_Connection*)&mock_connection));
}

static void test_client_accepts_brotli_brotli_with_quality(void) {
    // Test with brotli and quality value (br;q=0.8)
    mock_connection.accept_encoding = "gzip, br;q=0.8, deflate";
    TEST_ASSERT_TRUE(client_accepts_brotli((struct MHD_Connection*)&mock_connection));
}

static void test_client_accepts_brotli_case_sensitive(void) {
    // Test case sensitive matching - current implementation is case sensitive
    mock_connection.accept_encoding = "gzip, BR, deflate";
    TEST_ASSERT_FALSE(client_accepts_brotli((struct MHD_Connection*)&mock_connection));

    // But lowercase should work
    mock_connection.accept_encoding = "gzip, br, deflate";
    TEST_ASSERT_TRUE(client_accepts_brotli((struct MHD_Connection*)&mock_connection));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_client_accepts_brotli_null_connection);
    RUN_TEST(test_client_accepts_brotli_no_accept_encoding);
    RUN_TEST(test_client_accepts_brotli_empty_accept_encoding);
    RUN_TEST(test_client_accepts_brotli_brotli_not_supported);
    RUN_TEST(test_client_accepts_brotli_brotli_at_start);
    RUN_TEST(test_client_accepts_brotli_brotli_in_middle);
    RUN_TEST(test_client_accepts_brotli_brotli_at_end);
    RUN_TEST(test_client_accepts_brotli_brotli_only);
    RUN_TEST(test_client_accepts_brotli_brotli_with_quality);
    RUN_TEST(test_client_accepts_brotli_case_sensitive);

    return UNITY_END();
}
