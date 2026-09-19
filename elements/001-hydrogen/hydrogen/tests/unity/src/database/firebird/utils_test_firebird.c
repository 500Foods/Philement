/*
 * Unity Test File: Firebird Utility Functions
 * Tests firebird_build_attach_string, firebird_validate_connection_string,
 * firebird_escape_string, and firebird_parse_connstring_url.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebird/types.h>
#include <src/database/firebird/utils.h>

extern char* firebird_build_attach_string(const ConnectionConfig* config);
extern bool firebird_validate_connection_string(const char* connection_string);
extern char* firebird_escape_string(const DatabaseHandle* connection, const char* input);
extern bool firebird_parse_connstring_url(
    const char* conn_str,
    char* host_out, int host_out_sz,
    char* port_out, int port_out_sz,
    char* path_out, int path_out_sz,
    char* user_out, int user_out_sz,
    char* pass_out, int pass_out_sz,
    char* schema_out, int schema_out_sz);

// Test function prototypes
void test_firebird_validate_null(void);
void test_firebird_validate_empty(void);
void test_firebird_validate_firebird_url(void);
void test_firebird_validate_firebird_embedded(void);
void test_firebird_validate_dpb_string(void);
void test_firebird_validate_invalid(void);
void test_firebird_escape_string_with_quotes(void);
void test_firebird_escape_string_no_quotes(void);
void test_firebird_escape_null(void);
void test_firebird_build_attach_string(void);
void test_firebird_build_attach_string_null(void);
void test_firebird_parse_url_host_port_path(void);
void test_firebird_parse_url_embedded(void);
void test_firebird_parse_url_query_params(void);

void setUp(void) {
    // No fixtures needed
}

void tearDown(void) {
    // No cleanup needed
}

void test_firebird_validate_null(void) {
    TEST_ASSERT_FALSE(firebird_validate_connection_string(NULL));
}

void test_firebird_validate_empty(void) {
    TEST_ASSERT_FALSE(firebird_validate_connection_string(""));
}

void test_firebird_validate_firebird_url(void) {
    TEST_ASSERT_TRUE(firebird_validate_connection_string("firebird://localhost:3050/test.fdb"));
}

void test_firebird_validate_firebird_embedded(void) {
    TEST_ASSERT_TRUE(firebird_validate_connection_string("firebird:///var/lib/firebird/data/test.fdb"));
}

void test_firebird_validate_dpb_string(void) {
    TEST_ASSERT_TRUE(firebird_validate_connection_string("user = SYSDBA password = secret dbname = test.fdb"));
}

void test_firebird_validate_invalid(void) {
    TEST_ASSERT_FALSE(firebird_validate_connection_string("not a connection string"));
}

void test_firebird_escape_string_with_quotes(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBIRD;

    char* escaped = firebird_escape_string(&handle, "O'Brien");
    TEST_ASSERT_NOT_NULL(escaped);
    TEST_ASSERT_EQUAL_STRING("O''Brien", escaped);
    free(escaped);
}

void test_firebird_escape_string_no_quotes(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBIRD;

    char* escaped = firebird_escape_string(&handle, "plain_string");
    TEST_ASSERT_NOT_NULL(escaped);
    TEST_ASSERT_EQUAL_STRING("plain_string", escaped);
    free(escaped);
}

void test_firebird_escape_null(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBIRD;

    TEST_ASSERT_NULL(firebird_escape_string(&handle, NULL));
    TEST_ASSERT_NULL(firebird_escape_string(NULL, "input"));
}

void test_firebird_build_attach_string(void) {
    ConnectionConfig config = {0};
    config.username = strdup("SYSDBA");
    config.password = strdup("secret");
    config.database = strdup("/var/lib/firebird/data/test.fdb");
    config.schema = strdup("MYSCHEMA");;

    char* dpb = firebird_build_attach_string(&config);
    TEST_ASSERT_NOT_NULL(dpb);
    TEST_ASSERT_NOT_NULL(strstr(dpb, "user = SYSDBA"));
    TEST_ASSERT_NOT_NULL(strstr(dpb, "password = secret"));
    TEST_ASSERT_NOT_NULL(strstr(dpb, "dbname = /var/lib/firebird/data/test.fdb"));
    TEST_ASSERT_NOT_NULL(strstr(dpb, "schema = MYSCHEMA"));
    free(dpb);
}

void test_firebird_build_attach_string_null(void) {
    TEST_ASSERT_NULL(firebird_build_attach_string(NULL));
}

void test_firebird_parse_url_host_port_path(void) {
    char host[256], port[16], path[4096], user[256], pass[256], schema[256];

    bool result = firebird_parse_connstring_url(
        "firebird://myhost:3050/mydb/test.fdb",
        host, sizeof(host),
        port, sizeof(port),
        path, sizeof(path),
        user, sizeof(user),
        pass, sizeof(pass),
        schema, sizeof(schema));

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("myhost", host);
    TEST_ASSERT_EQUAL_STRING("3050", port);
    TEST_ASSERT_NOT_NULL(strstr(path, "test.fdb"));
}

void test_firebird_parse_url_embedded(void) {
    char host[256], port[16], path[4096], user[256], pass[256], schema[256];

    bool result = firebird_parse_connstring_url(
        "firebird:///var/lib/firebird/data/test.fdb",
        host, sizeof(host),
        port, sizeof(port),
        path, sizeof(path),
        user, sizeof(user),
        pass, sizeof(pass),
        schema, sizeof(schema));

    TEST_ASSERT_TRUE(result);
    // For embedded, path should contain the database path
    TEST_ASSERT_NOT_NULL(strstr(path, "test.fdb"));
}

void test_firebird_parse_url_query_params(void) {
    char host[256], port[16], path[4096], user[256], pass[256], schema[256];

    bool result = firebird_parse_connstring_url(
        "firebird://localhost:3050/test.fdb?user=SYSDBA&password=secretpass&schema=MYDB",
        host, sizeof(host),
        port, sizeof(port),
        path, sizeof(path),
        user, sizeof(user),
        pass, sizeof(pass),
        schema, sizeof(schema));

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("SYSDBA", user);
    TEST_ASSERT_EQUAL_STRING("secretpass", pass);
    TEST_ASSERT_EQUAL_STRING("MYDB", schema);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_firebird_validate_null);
    RUN_TEST(test_firebird_validate_empty);
    RUN_TEST(test_firebird_validate_firebird_url);
    RUN_TEST(test_firebird_validate_firebird_embedded);
    RUN_TEST(test_firebird_validate_dpb_string);
    RUN_TEST(test_firebird_validate_invalid);
    RUN_TEST(test_firebird_escape_string_with_quotes);
    RUN_TEST(test_firebird_escape_string_no_quotes);
    RUN_TEST(test_firebird_escape_null);
    RUN_TEST(test_firebird_build_attach_string);
    RUN_TEST(test_firebird_build_attach_string_null);
    RUN_TEST(test_firebird_parse_url_host_port_path);
    RUN_TEST(test_firebird_parse_url_embedded);
    RUN_TEST(test_firebird_parse_url_query_params);

    return UNITY_END();
}
