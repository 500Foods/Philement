/*
 * Unity Test File: MSSQL Utils Functions
 * This file contains unit tests for MSSQL utility functions (utils.c)
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/mssql/utils.h>

/* Forward declarations for functions being tested */
char* mssql_get_connection_string(const ConnectionConfig* config);
bool mssql_validate_connection_string(const char* connection_string);
void mssql_parse_connection_string(const char* conn_str, char* server, char* username, char* password, char* database);
char* mssql_escape_string(const DatabaseHandle* connection, const char* input);

/* USE_MOCK_SYSTEM is defined globally by CMake, but we need the header */
#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>

/* Test function prototypes */
void test_mssql_get_connection_string_null_config(void);
void test_mssql_get_connection_string_with_connection_string_field(void);
void test_mssql_get_connection_string_with_config_fields(void);
void test_mssql_get_connection_string_custom_port(void);
void test_mssql_get_connection_string_with_schema(void);
void test_mssql_get_connection_string_schema_strips_existing(void);
void test_mssql_get_connection_string_schema_strips_existing_no_semicolon(void);
void test_mssql_get_connection_string_schema_no_trailing_semicolon(void);
void test_mssql_get_connection_string_with_freetds_driver(void);
void test_mssql_get_connection_string_with_bracketed_driver(void);
void test_mssql_get_connection_string_null_host_database_username_password(void);

void test_mssql_validate_connection_string_null(void);
void test_mssql_validate_connection_string_empty(void);
void test_mssql_validate_connection_string_too_long(void);
void test_mssql_validate_connection_string_valid(void);
void test_mssql_validate_connection_string_missing_driver(void);
void test_mssql_validate_connection_string_missing_server(void);
void test_mssql_validate_connection_string_missing_both(void);

void test_mssql_parse_connection_string_null_params(void);
void test_mssql_parse_connection_string_basic(void);
void test_mssql_parse_connection_string_with_aliases(void);
void test_mssql_parse_connection_string_empty_string(void);
void test_mssql_parse_connection_string_no_equals(void);
void test_mssql_parse_connection_string_partial_fields(void);
void test_mssql_parse_connection_string_server_port_format(void);

void test_mssql_escape_string_null_connection(void);
void test_mssql_escape_string_null_input(void);
void test_mssql_escape_string_wrong_engine(void);
void test_mssql_escape_string_no_quotes(void);
void test_mssql_escape_string_with_quotes(void);
void test_mssql_escape_string_empty_string(void);
void test_mssql_escape_string_only_quotes(void);
void test_mssql_escape_string_all_quotes(void);

void setUp(void) {
    mock_system_reset_all();
    unsetenv("MSSQL_ODBC_DRIVER");
}

void tearDown(void) {
    mock_system_reset_all();
    unsetenv("MSSQL_ODBC_DRIVER");
}

/* ===== mssql_get_connection_string tests ===== */

void test_mssql_get_connection_string_null_config(void) {
    char* result = mssql_get_connection_string(NULL);
    TEST_ASSERT_NULL(result);
}

void test_mssql_get_connection_string_with_connection_string_field(void) {
    ConnectionConfig config = {0};
    config.connection_string = strdup("DRIVER={ODBC Driver 18 for SQL Server};SERVER=localhost;DATABASE=testdb;UID=testuser;PWD=testpass;");
    config.host = strdup("otherhost");
    config.port = 1433;
    config.database = strdup("otherdb");
    config.username = strdup("otheruser");
    config.password = strdup("otherpass");

    char* result = mssql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "DRIVER="));
    TEST_ASSERT_NOT_NULL(strstr(result, "SERVER=localhost"));
    TEST_ASSERT_NOT_NULL(strstr(result, "DATABASE=testdb"));
    TEST_ASSERT_NOT_NULL(strstr(result, "UID=testuser"));
    TEST_ASSERT_NOT_NULL(strstr(result, "PWD=testpass"));

    free(result);
    free(config.connection_string);
    free(config.host);
    free(config.database);
    free(config.username);
    free(config.password);
}

void test_mssql_get_connection_string_with_config_fields(void) {
    ConnectionConfig config = {0};
    config.host = strdup("myserver");
    config.port = 1433;
    config.database = strdup("mydb");
    config.username = strdup("myuser");
    config.password = strdup("mypass");

    char* result = mssql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "DRIVER={ODBC Driver 18 for SQL Server}"));
    TEST_ASSERT_NOT_NULL(strstr(result, "SERVER=myserver,1433"));
    TEST_ASSERT_NOT_NULL(strstr(result, "DATABASE=mydb"));
    TEST_ASSERT_NOT_NULL(strstr(result, "UID=myuser"));
    TEST_ASSERT_NOT_NULL(strstr(result, "PWD=mypass"));

    free(result);
    free(config.host);
    free(config.database);
    free(config.username);
    free(config.password);
}

void test_mssql_get_connection_string_custom_port(void) {
    ConnectionConfig config = {0};
    config.host = strdup("myserver");
    config.port = 1434;
    config.database = strdup("mydb");
    config.username = strdup("myuser");
    config.password = strdup("mypass");

    char* result = mssql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "SERVER=myserver,1434"));
    TEST_ASSERT_NULL(strstr(result, "SERVER=myserver,1433"));

    free(result);
    free(config.host);
    free(config.database);
    free(config.username);
    free(config.password);
}

void test_mssql_get_connection_string_null_host_database_username_password(void) {
    ConnectionConfig config = {0};
    config.host = NULL;
    config.port = 0;
    config.database = NULL;
    config.username = NULL;
    config.password = NULL;

    char* result = mssql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "SERVER=localhost,1433"));
    TEST_ASSERT_NOT_NULL(strstr(result, "DATABASE="));
    TEST_ASSERT_NOT_NULL(strstr(result, "UID="));
    TEST_ASSERT_NOT_NULL(strstr(result, "PWD="));

    free(result);
}

void test_mssql_get_connection_string_with_schema(void) {
    ConnectionConfig config = {0};
    config.host = strdup("myserver");
    config.port = 1433;
    config.database = strdup("mydb");
    config.username = strdup("myuser");
    config.password = strdup("mypass");
    config.schema = strdup("myschema");

    char* result = mssql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "SCHEMA=myschema;"));

    free(result);
    free(config.host);
    free(config.database);
    free(config.username);
    free(config.password);
    free(config.schema);
}

void test_mssql_get_connection_string_schema_strips_existing(void) {
    ConnectionConfig config = {0};
    config.connection_string = strdup("DRIVER={ODBC Driver 18};SERVER=localhost;SCHEMA=oldschema;DATABASE=testdb;");
    config.port = 1433;
    config.schema = strdup("newschema");

    char* result = mssql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "SCHEMA=newschema;"));
    TEST_ASSERT_NULL(strstr(result, "SCHEMA=oldschema;"));

    free(result);
    free(config.connection_string);
    free(config.schema);
}

void test_mssql_get_connection_string_schema_strips_existing_no_semicolon(void) {
    ConnectionConfig config = {0};
    config.connection_string = strdup("DRIVER={ODBC Driver 18};SERVER=localhost;DATABASE=testdb;SCHEMA=oldschema");
    config.port = 1433;
    config.schema = strdup("newschema");

    char* result = mssql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "SCHEMA=newschema;"));
    TEST_ASSERT_NULL(strstr(result, "SCHEMA=oldschema"));

    free(result);
    free(config.connection_string);
    free(config.schema);
}

void test_mssql_get_connection_string_schema_no_trailing_semicolon(void) {
    ConnectionConfig config = {0};
    config.connection_string = strdup("DRIVER={ODBC Driver 18};SERVER=localhost;DATABASE=testdb;FOOSCHEMA=oldschema");
    config.port = 1433;
    config.schema = strdup("newschema");

    char* result = mssql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "SCHEMA=newschema;"));
    TEST_ASSERT_NULL(strstr(result, "SCHEMA=oldschema"));

    free(result);
    free(config.connection_string);
    free(config.schema);
}

void test_mssql_get_connection_string_with_freetds_driver(void) {
    ConnectionConfig config = {0};
    config.host = strdup("myserver");
    config.port = 1433;
    config.database = strdup("mydb");
    config.username = strdup("myuser");
    config.password = strdup("mypass");

    setenv("MSSQL_ODBC_DRIVER", "FreeTDS", 1);
    char* result = mssql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "TDS_VERSION=7.4;ClientCharset=UTF-8;"));
    unsetenv("MSSQL_ODBC_DRIVER");

    free(result);
    free(config.host);
    free(config.database);
    free(config.username);
    free(config.password);
}

void test_mssql_get_connection_string_with_bracketed_driver(void) {
    ConnectionConfig config = {0};
    config.host = strdup("myserver");
    config.port = 1433;
    config.database = strdup("mydb");
    config.username = strdup("myuser");
    config.password = strdup("mypass");

    setenv("MSSQL_ODBC_DRIVER", "{My Custom Driver}", 1);
    char* result = mssql_get_connection_string(&config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "DRIVER={My Custom Driver}"));
    unsetenv("MSSQL_ODBC_DRIVER");

    free(result);
    free(config.host);
    free(config.database);
    free(config.username);
    free(config.password);
}

/* ===== mssql_validate_connection_string tests ===== */

void test_mssql_validate_connection_string_null(void) {
    bool result = mssql_validate_connection_string(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_mssql_validate_connection_string_empty(void) {
    bool result = mssql_validate_connection_string("");
    TEST_ASSERT_FALSE(result);
}

void test_mssql_validate_connection_string_too_long(void) {
    char long_str[4097];
    memset(long_str, 'a', 4096);
    long_str[4096] = '\0';
    bool result = mssql_validate_connection_string(long_str);
    TEST_ASSERT_FALSE(result);
}

void test_mssql_validate_connection_string_valid(void) {
    bool result = mssql_validate_connection_string("DRIVER={ODBC Driver 18 for SQL Server};SERVER=localhost;DATABASE=testdb;");
    TEST_ASSERT_TRUE(result);
}

void test_mssql_validate_connection_string_missing_driver(void) {
    bool result = mssql_validate_connection_string("SERVER=localhost;DATABASE=testdb;");
    TEST_ASSERT_FALSE(result);
}

void test_mssql_validate_connection_string_missing_server(void) {
    bool result = mssql_validate_connection_string("DRIVER={ODBC Driver 18 for SQL Server};DATABASE=testdb;");
    TEST_ASSERT_FALSE(result);
}

void test_mssql_validate_connection_string_missing_both(void) {
    bool result = mssql_validate_connection_string("DATABASE=testdb;");
    TEST_ASSERT_FALSE(result);
}

/* ===== mssql_parse_connection_string tests ===== */

void test_mssql_parse_connection_string_null_params(void) {
    mssql_parse_connection_string(NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_TRUE(true);
}

void test_mssql_parse_connection_string_basic(void) {
    char server[256] = {0};
    char username[128] = {0};
    char password[128] = {0};
    char database[128] = {0};

    mssql_parse_connection_string(
        "DRIVER={ODBC Driver 18};SERVER=myserver;UID=myuser;PWD=mypass;DATABASE=mydb",
        server, username, password, database);

    TEST_ASSERT_EQUAL_STRING("myserver", server);
    TEST_ASSERT_EQUAL_STRING("myuser", username);
    TEST_ASSERT_EQUAL_STRING("mypass", password);
    TEST_ASSERT_EQUAL_STRING("mydb", database);
}

void test_mssql_parse_connection_string_with_aliases(void) {
    char server[256] = {0};
    char username[128] = {0};
    char password[128] = {0};
    char database[128] = {0};

    mssql_parse_connection_string(
        "ADDR=otherhost;USER=otheruser;PASSWORD=otherpass;DB=otherdb",
        server, username, password, database);

    TEST_ASSERT_EQUAL_STRING("otherhost", server);
    TEST_ASSERT_EQUAL_STRING("otheruser", username);
    TEST_ASSERT_EQUAL_STRING("otherpass", password);
    TEST_ASSERT_EQUAL_STRING("otherdb", database);
}

void test_mssql_parse_connection_string_empty_string(void) {
    char server[256] = {0};
    char username[128] = {0};
    char password[128] = {0};
    char database[128] = {0};

    mssql_parse_connection_string("", server, username, password, database);

    TEST_ASSERT_EQUAL_STRING("", server);
    TEST_ASSERT_EQUAL_STRING("", username);
    TEST_ASSERT_EQUAL_STRING("", password);
    TEST_ASSERT_EQUAL_STRING("", database);
}

void test_mssql_parse_connection_string_no_equals(void) {
    char server[256] = {0};
    char username[128] = {0};
    char password[128] = {0};
    char database[128] = {0};

    mssql_parse_connection_string(
        "garbagetoken;DRIVER={ODBC Driver 18};SERVER=myserver",
        server, username, password, database);

    TEST_ASSERT_EQUAL_STRING("myserver", server);
    TEST_ASSERT_EQUAL_STRING("", username);
}

void test_mssql_parse_connection_string_partial_fields(void) {
    char server[256] = {0};
    char username[128] = {0};
    char password[128] = {0};
    char database[128] = {0};

    mssql_parse_connection_string(
        "SERVER=onlyserver",
        server, username, password, database);

    TEST_ASSERT_EQUAL_STRING("onlyserver", server);
    TEST_ASSERT_EQUAL_STRING("", username);
    TEST_ASSERT_EQUAL_STRING("", password);
    TEST_ASSERT_EQUAL_STRING("", database);
}

void test_mssql_parse_connection_string_server_port_format(void) {
    char server[256] = {0};
    char username[128] = {0};
    char password[128] = {0};
    char database[128] = {0};

    mssql_parse_connection_string(
        "SERVER=myserver,1433;UID=myuser;PWD=mypass;DATABASE=mydb",
        server, username, password, database);

    TEST_ASSERT_EQUAL_STRING("myserver,1433", server);
    TEST_ASSERT_EQUAL_STRING("myuser", username);
    TEST_ASSERT_EQUAL_STRING("mypass", password);
    TEST_ASSERT_EQUAL_STRING("mydb", database);
}

/* ===== mssql_escape_string tests ===== */

void test_mssql_escape_string_null_connection(void) {
    char* result = mssql_escape_string(NULL, "test");
    TEST_ASSERT_NULL(result);
}

void test_mssql_escape_string_null_input(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MSSQL;
    char* result = mssql_escape_string(&connection, NULL);
    TEST_ASSERT_NULL(result);
}

void test_mssql_escape_string_wrong_engine(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    char* result = mssql_escape_string(&connection, "test");
    TEST_ASSERT_NULL(result);
}

void test_mssql_escape_string_no_quotes(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MSSQL;
    char* result = mssql_escape_string(&connection, "hello world");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hello world", result);
    free(result);
}

void test_mssql_escape_string_with_quotes(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MSSQL;
    char* result = mssql_escape_string(&connection, "don't worry");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("don''t worry", result);
    free(result);
}

void test_mssql_escape_string_empty_string(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MSSQL;
    char* result = mssql_escape_string(&connection, "");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    free(result);
}

void test_mssql_escape_string_only_quotes(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MSSQL;
    char* result = mssql_escape_string(&connection, "''");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("''''", result);
    free(result);
}

void test_mssql_escape_string_all_quotes(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_MSSQL;
    char* result = mssql_escape_string(&connection, "'''");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("''''''", result);
    free(result);
}

int main(void) {
    UNITY_BEGIN();

    /* mssql_get_connection_string tests */
    RUN_TEST(test_mssql_get_connection_string_null_config);
    RUN_TEST(test_mssql_get_connection_string_with_connection_string_field);
    RUN_TEST(test_mssql_get_connection_string_with_config_fields);
    RUN_TEST(test_mssql_get_connection_string_custom_port);
    RUN_TEST(test_mssql_get_connection_string_null_host_database_username_password);
    RUN_TEST(test_mssql_get_connection_string_with_schema);
    RUN_TEST(test_mssql_get_connection_string_schema_strips_existing);
    RUN_TEST(test_mssql_get_connection_string_schema_strips_existing_no_semicolon);
    RUN_TEST(test_mssql_get_connection_string_schema_no_trailing_semicolon);
    RUN_TEST(test_mssql_get_connection_string_with_freetds_driver);
    RUN_TEST(test_mssql_get_connection_string_with_bracketed_driver);

    /* mssql_validate_connection_string tests */
    RUN_TEST(test_mssql_validate_connection_string_null);
    RUN_TEST(test_mssql_validate_connection_string_empty);
    RUN_TEST(test_mssql_validate_connection_string_too_long);
    RUN_TEST(test_mssql_validate_connection_string_valid);
    RUN_TEST(test_mssql_validate_connection_string_missing_driver);
    RUN_TEST(test_mssql_validate_connection_string_missing_server);
    RUN_TEST(test_mssql_validate_connection_string_missing_both);

    /* mssql_parse_connection_string tests */
    RUN_TEST(test_mssql_parse_connection_string_null_params);
    RUN_TEST(test_mssql_parse_connection_string_basic);
    RUN_TEST(test_mssql_parse_connection_string_with_aliases);
    RUN_TEST(test_mssql_parse_connection_string_empty_string);
    RUN_TEST(test_mssql_parse_connection_string_no_equals);
    RUN_TEST(test_mssql_parse_connection_string_partial_fields);
    RUN_TEST(test_mssql_parse_connection_string_server_port_format);

    /* mssql_escape_string tests */
    RUN_TEST(test_mssql_escape_string_null_connection);
    RUN_TEST(test_mssql_escape_string_null_input);
    RUN_TEST(test_mssql_escape_string_wrong_engine);
    RUN_TEST(test_mssql_escape_string_no_quotes);
    RUN_TEST(test_mssql_escape_string_with_quotes);
    RUN_TEST(test_mssql_escape_string_empty_string);
    RUN_TEST(test_mssql_escape_string_only_quotes);
    RUN_TEST(test_mssql_escape_string_all_quotes);

    return UNITY_END();
}
