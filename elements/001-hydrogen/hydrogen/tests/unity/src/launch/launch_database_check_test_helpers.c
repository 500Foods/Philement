/*
 * Unity tests for launch_database_check.c: add_connection_name, report_database_count, and check_database_library.
 *
 * CHANGELOG
 * 4.0.0 - 2026-10-02 - Split from launch_database_check_test_edges.c to keep test files under 1000 lines.
 *                       Tests for add_connection_name, report_database_count, and check_database_library.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/launch/launch.h>

// Standard library includes
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Enable mocks for comprehensive testing
#define USE_MOCK_LIBPQ
#define USE_MOCK_LIBMYSQLCLIENT
#define USE_MOCK_LIBSQLITE3
#define USE_MOCK_LIBDB2
#define USE_MOCK_SYSTEM
#define USE_MOCK_LAUNCH

#include <unity/mocks/mock_libpq.h>
#include <unity/mocks/mock_libmysqlclient.h>
#include <unity/mocks/mock_libsqlite3.h>
#include <unity/mocks/mock_libdb2.h>
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_launch.h>
#include <unity/mocks/mock_logging.h>
#include <unity/mocks/mock_libmicrohttpd.h>

// Forward declarations for test functions in this file
void test_add_connection_name_null_seed(void);
void test_add_connection_name_append(void);
void test_add_connection_name_null_new_name(void);
void test_add_connection_name_existing_with_null_new(void);
void test_add_connection_name_realloc_failure(void);
void test_report_database_count_with_names(void);
void test_report_database_count_truncation(void);
void test_report_database_count_null_names(void);
void test_check_database_library_success_with_version(void);
void test_check_database_library_success_no_expected(void);
void test_check_database_library_failure(void);
void test_check_database_library_keep_open(void);

// Forward declarations for tests in other split files
void test_check_database_library_dependencies_postgres(void);
extern volatile sig_atomic_t database_stopping;
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_running;
extern volatile sig_atomic_t server_stopping;

// Test data structures
static AppConfig test_app_config;

void setUp(void) {
    // Reset global state
    server_stopping = 0;
    server_starting = 1;
    server_running = 0;
    database_stopping = 0;
    app_config = &test_app_config;
    mock_system_reset_all();
    mock_launch_reset_all();

    // Set up default valid config
    memset(&test_app_config, 0, sizeof(AppConfig));
    // Initialize with minimal database config
    test_app_config.databases.connection_count = 0;
    // connections array is already zeroed by memset
}

void tearDown(void) {
    // Clean up
}

// ============================================================================
// add_connection_name tests
// ============================================================================

// Test with NULL names (first call - seeds the buffer via strdup)
void test_add_connection_name_null_seed(void) {
    char* result = add_connection_name(NULL, "test_name");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test_name", result);
    free(result);
}

// Test appending to existing names list
void test_add_connection_name_append(void) {
    char* result = add_connection_name(NULL, "name1");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("name1", result);

    result = add_connection_name(result, "name2");
    TEST_ASSERT_EQUAL_STRING("name1, name2", result);

    result = add_connection_name(result, "name3");
    TEST_ASSERT_EQUAL_STRING("name1, name2, name3", result);

    free(result);
}

// Test with NULL new_name (should use "Unknown")
void test_add_connection_name_null_new_name(void) {
    char* result = add_connection_name(NULL, NULL);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("Unknown", result);

    // Append with NULL name
    result = add_connection_name(result, NULL);
    TEST_ASSERT_EQUAL_STRING("Unknown, Unknown", result);

    free(result);
}

// Test existing names with NULL new_name
void test_add_connection_name_existing_with_null_new(void) {
    char* result = add_connection_name(NULL, "existing");
    result = add_connection_name(result, NULL);
    TEST_ASSERT_EQUAL_STRING("existing, Unknown", result);
    free(result);
}

// Test realloc failure path (when names is non-NULL, realloc returns NULL)
void test_add_connection_name_realloc_failure(void) {
    // Create initial names to make it non-NULL
    char* names = strdup("name1");

    // Set up realloc to fail
    mock_system_set_realloc_failure(1);

    char* result = add_connection_name(names, "name2");

    // On realloc failure, function returns original names pointer
    TEST_ASSERT_EQUAL_PTR(names, result);

    // Reset realloc mock
    mock_system_set_realloc_failure(0);

    free(result);
}

// ============================================================================
// report_database_count tests
// ============================================================================

// Test reporting with names and count <= 3
void test_report_database_count_with_names(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    report_database_count(&messages, &count, &capacity, "PostgreSQL Databases", 2, strdup("pg1, pg2"));

    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_NOT_NULL(messages[0]);
    TEST_ASSERT_EQUAL_STRING("  Go:      PostgreSQL Databases: 2 (pg1, pg2)", messages[0]);

    // Clean up
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test reporting with count > 3 (truncation path)
void test_report_database_count_truncation(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    // Create a long names string (> 50 chars to test truncation)
    char* long_names = strdup("conn1, conn2, conn3, conn4, conn5, conn6, conn7, conn8, conn9, conn10");

    report_database_count(&messages, &count, &capacity, "PostgreSQL Databases", 4, long_names);

    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_NOT_NULL(messages[0]);
    // db_names[50] = 0 truncates to first 50 chars, then "..." is appended
    TEST_ASSERT_EQUAL_STRING("  Go:      PostgreSQL Databases: 4 (conn1, conn2, conn3, conn4, conn5, conn6, conn7, c...)", messages[0]);

    // Clean up
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test reporting with NULL db_names (no names path)
void test_report_database_count_null_names(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;

    report_database_count(&messages, &count, &capacity, "PostgreSQL Databases", 3, NULL);

    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_NOT_NULL(messages[0]);
    TEST_ASSERT_EQUAL_STRING("  Go:      PostgreSQL Databases: 3", messages[0]);

    // Clean up
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// ============================================================================
// check_database_library tests
// ============================================================================

// Test successful library load with version and expected_version (minor match)
void test_check_database_library_success_with_version(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    // Mock dlopen to succeed
    void* fake_handle = (void*)0xDEADBEEF;
    mock_system_set_dlopen_result(fake_handle);

    const char* lib_paths[] = {"libpq.so.5"};
    bool result = check_database_library(&messages, &count, &capacity, &overall_readiness,
                                        "PostgreSQL", lib_paths, 1, "libpq.so",
                                        "17.6", "minor", false);

    // Library loaded successfully (version will be version-unknown due to dlsym mock)
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(overall_readiness);
    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_GREATER_THAN(0, count);

    // Clean up
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test successful library load with version but no expected_version
void test_check_database_library_success_no_expected(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    void* fake_handle = (void*)0x12345678;
    mock_system_set_dlopen_result(fake_handle);

    const char* lib_paths[] = {"libmariadb.so.3"};
    bool result = check_database_library(&messages, &count, &capacity, &overall_readiness,
                                        "MariaDB", lib_paths, 1, "libmariadb.so",
                                        NULL, NULL, false);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(overall_readiness);
    TEST_ASSERT_GREATER_THAN(0, count);

    // Clean up
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test library load failure
void test_check_database_library_failure(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    // Mock dlopen to fail
    mock_system_set_dlopen_failure(1);

    const char* lib_paths[] = {"nonexistent.so"};
    bool result = check_database_library(&messages, &count, &capacity, &overall_readiness,
                                        "TestLib", lib_paths, 1, "testlib.so",
                                        NULL, NULL, false);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(overall_readiness);
    TEST_ASSERT_NOT_NULL(messages);
    TEST_ASSERT_GREATER_THAN(0, count);

    // Reset mock
    mock_system_set_dlopen_failure(0);
    mock_system_set_dlerror_result("Library not loaded");

    // Clean up
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Test keep_open=true (library handle not closed)
void test_check_database_library_keep_open(void) {
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    bool overall_readiness = true;

    void* fake_handle = (void*)0xABCDEF00;
    mock_system_set_dlopen_result(fake_handle);

    const char* lib_paths[] = {"libfbclient.so.2"};
    bool result = check_database_library(&messages, &count, &capacity, &overall_readiness,
                                        "Firebird", lib_paths, 1, "libfbclient.so",
                                        NULL, NULL, true);  // keep_open=true

    TEST_ASSERT_TRUE(result);

    // Clean up
    for (size_t i = 0; i < count; i++) {
        free((void*)messages[i]);
    }
    free(messages);
}

// Forward declarations for tests in the other split files
void test_check_database_library_dependencies_postgres(void);
void test_check_database_library_dependencies_mysql(void);
void test_check_database_library_dependencies_db2(void);
void test_check_database_library_dependencies_sqlite(void);
void test_check_database_library_dependencies_mariadb(void);
void test_check_database_library_dependencies_firebird(void);
void test_check_database_library_dependencies_mssql(void);
void test_check_database_library_dependencies_zero(void);
void test_validate_database_configuration_multiple_same_type(void);
void test_validate_database_configuration_multiple_postgres(void);
void test_validate_database_configuration_multiple_mysql(void);
void test_validate_database_configuration_multiple_sqlite(void);
void test_validate_database_configuration_multiple_db2(void);
void test_validate_database_configuration_multiple_mariadb(void);
void test_validate_database_configuration_multiple_firebird(void);
void test_validate_database_configuration_multiple_mssql(void);
void test_validate_database_configuration_truncation(void);
void test_validate_database_configuration_null_connection_name(void);
void test_validate_database_configuration_disabled_connection(void);
void test_validate_database_configuration_zero_connections(void);
void test_validate_database_configuration_unknown_type(void);
void test_validate_database_configuration_null_type(void);
void test_validate_database_configuration_postgres_alias(void);
void test_validate_database_configuration_mixed_types(void);
void test_validate_database_connections_invalid_name(void);
void test_validate_database_connections_invalid_type(void);
void test_validate_database_connections_missing_sqlite_database(void);
void test_validate_database_connections_sqlite_file_not_found(void);
void test_validate_database_connections_missing_fields_non_sqlite(void);
void test_validate_database_connections_disabled(void);
void test_validate_database_connections_valid_sqlite(void);
void test_validate_database_connections_valid_non_sqlite(void);
void test_validate_database_connections_null_name(void);
void test_validate_database_connections_empty_name(void);
void test_validate_database_connections_short_name_valid(void);
void test_validate_database_connections_max_length_name(void);
void test_validate_database_connections_empty_type(void);
void test_validate_database_connections_zero_count(void);
void test_check_database_launch_readiness_dependency_failures(void);
void test_check_database_launch_readiness_already_registered(void);
void test_check_database_launch_readiness_invalid_connections(void);
void test_check_database_launch_readiness_library_dependency_missing(void);
void test_launch_database_subsystem_with_mocks(void);

int main(void) {
    UNITY_BEGIN();

    // add_connection_name tests
    RUN_TEST(test_add_connection_name_null_seed);
    RUN_TEST(test_add_connection_name_append);
    RUN_TEST(test_add_connection_name_null_new_name);
    RUN_TEST(test_add_connection_name_existing_with_null_new);
    RUN_TEST(test_add_connection_name_realloc_failure);

    // report_database_count tests
    RUN_TEST(test_report_database_count_with_names);
    RUN_TEST(test_report_database_count_truncation);
    RUN_TEST(test_report_database_count_null_names);

    // check_database_library tests
    RUN_TEST(test_check_database_library_success_with_version);
    RUN_TEST(test_check_database_library_success_no_expected);
    RUN_TEST(test_check_database_library_failure);
    RUN_TEST(test_check_database_library_keep_open);

    return UNITY_END();
}
