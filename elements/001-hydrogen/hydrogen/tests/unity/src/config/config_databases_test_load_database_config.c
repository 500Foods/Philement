/*
 * Unity Test File: Database Configuration Tests
 * This file contains unit tests for the load_database_config function
 * from src/config/config_databases.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/config/config_databases.h>
#include <src/config/config.h>

// Forward declarations for functions being tested
bool load_database_config(json_t* root, AppConfig* config);
void cleanup_database_connection(DatabaseConnection* conn);
void cleanup_database_config(DatabaseConfig* config);
void dump_database_config(const DatabaseConfig* config);

// Forward declarations for helper functions we'll need
bool initialize_config_defaults(AppConfig* config);

// Forward declarations for test functions
void test_load_database_config_null_root(void);
void test_load_database_config_empty_json(void);
void test_load_database_config_basic_fields(void);
void test_load_database_config_connection_count(void);
void test_cleanup_database_connection_null_pointer(void);
void test_cleanup_database_connection_empty_config(void);
void test_cleanup_database_connection_with_data(void);
void test_cleanup_database_config_null_pointer(void);
void test_cleanup_database_config_empty_config(void);
void test_cleanup_database_config_with_data(void);
void test_dump_database_config_null_pointer(void);
void test_dump_database_config_basic(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== PARAMETER VALIDATION TESTS =====

void test_load_database_config_null_root(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Test with NULL root - should handle gracefully and use defaults
    bool result = load_database_config(NULL, &config);

    // Function should initialize defaults and return success even with NULL root
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, config.databases.connection_count);  // No connections
}

void test_load_database_config_empty_json(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    bool result = load_database_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, config.databases.connection_count);  // No connections

    json_decref(root);
    cleanup_database_config(&config.databases);
}

// ===== BASIC FIELD TESTS =====

void test_load_database_config_basic_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* databases_section = json_object();
    json_t* connections_array = json_array();
    json_t* connection_obj = json_object();

    // Set up basic database configuration
    json_object_set(connection_obj, "Name", json_string("TestDB"));
    json_object_set(connection_obj, "Enabled", json_true());
    json_object_set(connection_obj, "Type", json_string("sqlite"));
    json_object_set(connection_obj, "Database", json_string("test.db"));

    json_array_append(connections_array, connection_obj);
    json_object_set(databases_section, "Connections", connections_array);
    json_object_set(root, "Databases", databases_section);

    bool result = load_database_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, config.databases.connection_count);
    TEST_ASSERT_EQUAL_STRING("TestDB", config.databases.connections[0].connection_name);
    TEST_ASSERT_TRUE(config.databases.connections[0].enabled);
    TEST_ASSERT_EQUAL_STRING("sqlite", config.databases.connections[0].type);
    TEST_ASSERT_EQUAL_STRING("test.db", config.databases.connections[0].database);

    json_decref(root);
    cleanup_database_config(&config.databases);
}

void test_load_database_config_connection_count(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* databases_section = json_object();
    json_t* connections_array = json_array();

    // Test custom connection count with connections array
    json_object_set(databases_section, "ConnectionCount", json_integer(3));
    json_object_set(databases_section, "Connections", connections_array);
    json_object_set(root, "Databases", databases_section);

    bool result = load_database_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, config.databases.connection_count);  // Empty connections array = 0 valid connections

    json_decref(root);
    cleanup_database_config(&config.databases);
}

// ===== CLEANUP FUNCTION TESTS =====

void test_cleanup_database_connection_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully
    cleanup_database_connection(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_database_connection_empty_config(void) {
    DatabaseConnection conn = {0};

    // Test cleanup on empty/uninitialized connection
    cleanup_database_connection(&conn);

    // Connection should be zeroed out
    TEST_ASSERT_NULL(conn.name);
    TEST_ASSERT_NULL(conn.connection_name);
    TEST_ASSERT_NULL(conn.type);
    TEST_ASSERT_NULL(conn.database);
    TEST_ASSERT_NULL(conn.host);
    TEST_ASSERT_NULL(conn.port);
    TEST_ASSERT_NULL(conn.user);
    TEST_ASSERT_NULL(conn.pass);
    TEST_ASSERT_NULL(conn.bootstrap_query);
}

void test_cleanup_database_connection_with_data(void) {
    DatabaseConnection conn = {0};

    // Initialize with some test data
    conn.name = strdup("test-db");
    conn.connection_name = strdup("TestDB");
    conn.type = strdup("sqlite");
    conn.database = strdup("test.db");
    conn.host = strdup("localhost");
    conn.port = strdup("5432");
    conn.user = strdup("testuser");
    conn.pass = strdup("testpass");
    conn.bootstrap_query = strdup("SELECT 1");

    // Cleanup should free all allocated memory
    cleanup_database_connection(&conn);

    // Connection should be zeroed out
    TEST_ASSERT_NULL(conn.name);
    TEST_ASSERT_NULL(conn.connection_name);
    TEST_ASSERT_NULL(conn.type);
    TEST_ASSERT_NULL(conn.database);
    TEST_ASSERT_NULL(conn.host);
    TEST_ASSERT_NULL(conn.port);
    TEST_ASSERT_NULL(conn.user);
    TEST_ASSERT_NULL(conn.pass);
    TEST_ASSERT_NULL(conn.bootstrap_query);
}

void test_cleanup_database_config_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully
    cleanup_database_config(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_database_config_empty_config(void) {
    DatabaseConfig config = {0};

    // Test cleanup on empty/uninitialized config
    cleanup_database_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_EQUAL(0, config.connection_count);
}

void test_cleanup_database_config_with_data(void) {
    DatabaseConfig config = {0};

    // Initialize with some test data
    config.connection_count = 1;
    config.connections[0].name = strdup("test-db");
    config.connections[0].connection_name = strdup("TestDB");
    config.connections[0].type = strdup("sqlite");
    config.connections[0].database = strdup("test.db");

    // Cleanup should free all allocated memory
    cleanup_database_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_EQUAL(0, config.connection_count);
    TEST_ASSERT_NULL(config.connections[0].name);
    TEST_ASSERT_NULL(config.connections[0].connection_name);
    TEST_ASSERT_NULL(config.connections[0].type);
    TEST_ASSERT_NULL(config.connections[0].database);
}

// ===== DUMP FUNCTION TESTS =====

void test_dump_database_config_null_pointer(void) {
    // Test dump with NULL pointer - should handle gracefully
    dump_database_config(NULL);
    // No assertions needed - function should not crash
}

void test_dump_database_config_basic(void) {
    DatabaseConfig config = {0};

    // Initialize with test data
    config.connection_count = 1;
    config.connections[0].name = strdup("test-db");
    config.connections[0].connection_name = strdup("TestDB");
    config.connections[0].enabled = true;
    config.connections[0].type = strdup("sqlite");
    config.connections[0].database = strdup("test.db");

    // Dump should not crash and handle the data properly
    dump_database_config(&config);

    // Clean up
    cleanup_database_config(&config);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_load_database_config_null_root);
    RUN_TEST(test_load_database_config_empty_json);

    // Basic field tests
    RUN_TEST(test_load_database_config_basic_fields);
    RUN_TEST(test_load_database_config_connection_count);

    // Cleanup function tests
    RUN_TEST(test_cleanup_database_connection_null_pointer);
    RUN_TEST(test_cleanup_database_connection_empty_config);
    RUN_TEST(test_cleanup_database_connection_with_data);
    RUN_TEST(test_cleanup_database_config_null_pointer);
    RUN_TEST(test_cleanup_database_config_empty_config);
    RUN_TEST(test_cleanup_database_config_with_data);

    // Dump function tests
    RUN_TEST(test_dump_database_config_null_pointer);
    RUN_TEST(test_dump_database_config_basic);

    return UNITY_END();
}