/*
 * Unity Test File: database_engine_metrics_test_coverage
 * This file contains unit tests for the database engine metrics helpers
 * (defined in src/database/database_engine_metrics.c):
 *   - database_get_supported_engines()
 *   - database_get_counts_by_type()
 * These were previously housed in database_test_coverage_improvement.c but
 * are relocated here so the build system attributes them to the correct
 * source file (database_engine_metrics.c).
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>

// Function prototypes for test functions
void test_database_get_supported_engines_parameter_validation(void);
void test_database_get_supported_engines_null_buffer(void);
void test_database_get_supported_engines_zero_buffer_size(void);
void test_database_get_supported_engines_null_subsystem(void);
void test_database_get_supported_engines_valid(void);
void test_database_get_counts_by_type_no_config(void);
void test_database_get_counts_by_type_with_config(void);

void setUp(void) {
    // Initialize database subsystem for testing
    database_subsystem_init();
}

void tearDown(void) {
    // Clean up test fixtures
    database_subsystem_shutdown();
}

// Test database_get_supported_engines parameter validation
void test_database_get_supported_engines_parameter_validation(void) {
    char buffer[256];

    // Test NULL buffer
    database_get_supported_engines(NULL, sizeof(buffer));
    // Should not crash

    // Test zero buffer size
    database_get_supported_engines(buffer, 0);
    // Should not crash

    // Test NULL subsystem
    DatabaseSubsystem* saved_subsystem = database_subsystem;
    database_subsystem = NULL;

    database_get_supported_engines(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(strstr(buffer, "not initialized") != NULL);

    database_subsystem = saved_subsystem;

    // Test valid parameters
    database_get_supported_engines(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(strlen(buffer) > 0);
    TEST_ASSERT_TRUE(strstr(buffer, "PostgreSQL") != NULL);
}

// Test database_get_supported_engines with NULL buffer
void test_database_get_supported_engines_null_buffer(void) {
    // Should not crash with NULL buffer
    database_get_supported_engines(NULL, 256);
    // Test passes if no crash occurs
}

// Test database_get_supported_engines with zero buffer size
void test_database_get_supported_engines_zero_buffer_size(void) {
    char buffer[256];
    // Should not crash with zero buffer size
    database_get_supported_engines(buffer, 0);
    // Test passes if no crash occurs
}

// Test database_get_supported_engines with NULL subsystem
void test_database_get_supported_engines_null_subsystem(void) {
    char buffer[256];
    // Save original subsystem
    DatabaseSubsystem* saved_subsystem = database_subsystem;
    database_subsystem = NULL;

    database_get_supported_engines(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(strstr(buffer, "not initialized") != NULL);

    // Restore
    database_subsystem = saved_subsystem;
}

// Test database_get_supported_engines with valid parameters
void test_database_get_supported_engines_valid(void) {
    char buffer[256];
    database_get_supported_engines(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(strlen(buffer) > 0);
    TEST_ASSERT_TRUE(strstr(buffer, "PostgreSQL") != NULL);
}

// Test database_get_counts_by_type with no config
void test_database_get_counts_by_type_no_config(void) {
    // Save original config
    AppConfig* saved_config = app_config;
    app_config = NULL;

    int postgres_count, mysql_count, sqlite_count, db2_count;
    database_get_counts_by_type(&postgres_count, &mysql_count, &sqlite_count, &db2_count);

    TEST_ASSERT_EQUAL(0, postgres_count);
    TEST_ASSERT_EQUAL(0, mysql_count);
    TEST_ASSERT_EQUAL(0, sqlite_count);
    TEST_ASSERT_EQUAL(0, db2_count);

    app_config = saved_config;
}

// Test database_get_counts_by_type with config
void test_database_get_counts_by_type_with_config(void) {
    // This would require setting up app_config with database connections
    // For now, just test that it doesn't crash
    int postgres_count, mysql_count, sqlite_count, db2_count;
    database_get_counts_by_type(&postgres_count, &mysql_count, &sqlite_count, &db2_count);

    // Values depend on configuration
    TEST_ASSERT_TRUE(postgres_count >= 0);
    TEST_ASSERT_TRUE(mysql_count >= 0);
    TEST_ASSERT_TRUE(sqlite_count >= 0);
    TEST_ASSERT_TRUE(db2_count >= 0);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_get_supported_engines_parameter_validation);
    RUN_TEST(test_database_get_supported_engines_null_buffer);
    RUN_TEST(test_database_get_supported_engines_zero_buffer_size);
    RUN_TEST(test_database_get_supported_engines_null_subsystem);
    RUN_TEST(test_database_get_supported_engines_valid);
    RUN_TEST(test_database_get_counts_by_type_no_config);
    RUN_TEST(test_database_get_counts_by_type_with_config);

    return UNITY_END();
}
