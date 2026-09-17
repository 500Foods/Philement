/*
 * Unity tests for Firebase connection-string and escape helpers.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebase/utils.h>
#include <src/database/firebase/types.h>

void test_firebase_get_connection_string_null_config(void);
void test_firebase_get_connection_string_from_fields(void);
void test_firebase_get_connection_string_existing(void);
void test_firebase_validate_connection_string_null(void);
void test_firebase_validate_connection_string_empty(void);
void test_firebase_validate_connection_string_valid(void);
void test_firebase_validate_connection_string_invalid(void);
void test_firebase_escape_string_null_connection(void);
void test_firebase_escape_string_wrong_engine(void);
void test_firebase_escape_string_quotes(void);
void test_firebase_config_is_emulator_loopback(void);
void test_firebase_host_is_production(void);
void test_firebase_config_is_emulator_variants(void);
void test_firebase_get_connection_string_missing_project(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_get_connection_string_null_config(void) {
    TEST_ASSERT_NULL(firebase_get_connection_string(NULL));
}

void test_firebase_get_connection_string_from_fields(void) {
    ConnectionConfig config = {0};
    config.username = (char*)"hydrodemo";
    config.host = (char*)"127.0.0.1";
    config.port = 8080;
    config.database = (char*)"(default)";

    char* result = firebase_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "firebase://hydrodemo/(default)"));
    TEST_ASSERT_NOT_NULL(strstr(result, "host=127.0.0.1"));
    TEST_ASSERT_NOT_NULL(strstr(result, "port=8080"));
    TEST_ASSERT_NOT_NULL(strstr(result, "emulator=1"));
    TEST_ASSERT_NULL(strstr(result, "pass="));
    free(result);
}

void test_firebase_get_connection_string_existing(void) {
    ConnectionConfig config = {0};
    config.connection_string = (char*)"firebase://proj/(default)?host=127.0.0.1&port=8080&emulator=1";
    char* result = firebase_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(config.connection_string, result);
    free(result);
}

void test_firebase_validate_connection_string_null(void) {
    TEST_ASSERT_FALSE(firebase_validate_connection_string(NULL));
}

void test_firebase_validate_connection_string_empty(void) {
    TEST_ASSERT_FALSE(firebase_validate_connection_string(""));
    TEST_ASSERT_FALSE(firebase_validate_connection_string("firebase://"));
    TEST_ASSERT_FALSE(firebase_validate_connection_string("firebase://proj"));
    TEST_ASSERT_FALSE(firebase_validate_connection_string("firebase://proj/"));
}

void test_firebase_validate_connection_string_valid(void) {
    TEST_ASSERT_TRUE(firebase_validate_connection_string(
        "firebase://hydrodemo/(default)?host=127.0.0.1&port=8080&emulator=1"));
}

void test_firebase_validate_connection_string_invalid(void) {
    TEST_ASSERT_FALSE(firebase_validate_connection_string("postgresql://x"));
    TEST_ASSERT_FALSE(firebase_validate_connection_string("sqlite.db"));
}

void test_firebase_escape_string_null_connection(void) {
    TEST_ASSERT_NULL(firebase_escape_string(NULL, "abc"));
}

void test_firebase_escape_string_wrong_engine(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_SQLITE;
    TEST_ASSERT_NULL(firebase_escape_string(&handle, "abc"));
}

void test_firebase_escape_string_quotes(void) {
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBASE;
    char* escaped = firebase_escape_string(&handle, "O'Reilly");
    TEST_ASSERT_NOT_NULL(escaped);
    TEST_ASSERT_EQUAL_STRING("O''Reilly", escaped);
    free(escaped);
}

void test_firebase_config_is_emulator_loopback(void) {
    ConnectionConfig config = {0};
    config.host = (char*)"127.0.0.1";
    config.port = 8080;
    TEST_ASSERT_TRUE(firebase_config_is_emulator(&config));

    ConnectionConfig prod = {0};
    prod.host = (char*)"firestore.googleapis.com";
    prod.port = 443;
    TEST_ASSERT_FALSE(firebase_config_is_emulator(&prod));
}

void test_firebase_host_is_production(void) {
    TEST_ASSERT_TRUE(firebase_host_is_production("firestore.googleapis.com"));
    TEST_ASSERT_FALSE(firebase_host_is_production("127.0.0.1"));
    TEST_ASSERT_FALSE(firebase_host_is_production(NULL));
}

void test_firebase_config_is_emulator_variants(void) {
    TEST_ASSERT_FALSE(firebase_config_is_emulator(NULL));

    ConnectionConfig via_flag = {0};
    via_flag.host = (char*)"10.0.0.8";
    via_flag.port = 443;
    via_flag.connection_string = (char*)"firebase://p/(default)?host=10.0.0.8&port=443&emulator=1";
    TEST_ASSERT_TRUE(firebase_config_is_emulator(&via_flag));

    ConnectionConfig localhost = {0};
    localhost.host = (char*)"localhost";
    TEST_ASSERT_TRUE(firebase_config_is_emulator(&localhost));

    ConnectionConfig ipv6 = {0};
    ipv6.host = (char*)"::1";
    TEST_ASSERT_TRUE(firebase_config_is_emulator(&ipv6));

    ConnectionConfig by_port = {0};
    by_port.host = (char*)"10.0.0.8";
    by_port.port = 8080;
    TEST_ASSERT_TRUE(firebase_config_is_emulator(&by_port));

    ConnectionConfig empty_host = {0};
    TEST_ASSERT_TRUE(firebase_config_is_emulator(&empty_host));
}

void test_firebase_get_connection_string_missing_project(void) {
    ConnectionConfig config = {0};
    config.host = (char*)"127.0.0.1";
    TEST_ASSERT_NULL(firebase_get_connection_string(&config));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_get_connection_string_null_config);
    RUN_TEST(test_firebase_get_connection_string_from_fields);
    RUN_TEST(test_firebase_get_connection_string_existing);
    RUN_TEST(test_firebase_validate_connection_string_null);
    RUN_TEST(test_firebase_validate_connection_string_empty);
    RUN_TEST(test_firebase_validate_connection_string_valid);
    RUN_TEST(test_firebase_validate_connection_string_invalid);
    RUN_TEST(test_firebase_escape_string_null_connection);
    RUN_TEST(test_firebase_escape_string_wrong_engine);
    RUN_TEST(test_firebase_escape_string_quotes);
    RUN_TEST(test_firebase_config_is_emulator_loopback);
    RUN_TEST(test_firebase_host_is_production);
    RUN_TEST(test_firebase_config_is_emulator_variants);
    RUN_TEST(test_firebase_get_connection_string_missing_project);
    return UNITY_END();
}
