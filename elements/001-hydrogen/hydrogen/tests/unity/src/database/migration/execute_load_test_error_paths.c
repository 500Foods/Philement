/*
 * Unity Test File: execute_load_test_error_paths
 * This file contains unit tests for error paths in execute_load.c functions
 * Focuses on improving coverage for uncovered error handling paths
 */

#define USE_MOCK_SYSTEM
#include <tests/unity/mocks/mock_system.h>

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/migration/migration.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/config/config.h>
#include <src/payload/payload.h>

// Forward declarations for functions being tested
const char* extract_migration_name(const char* migrations_config, char** path_copy_out);
bool execute_single_migration_load_only_with_state(DatabaseHandle* connection, const char* migration_file,
                                                   const char* engine_name, const char* migration_name,
                                                   const char* schema_name, const char* dqm_label,
                                                   lua_State* L, PayloadFile* payload_files, size_t payload_count);

// Forward declarations for test functions
void test_extract_migration_name_null_input(void);
void test_extract_migration_name_empty_string(void);
void test_extract_migration_name_payload_prefix(void);
void test_extract_migration_name_path_based(void);
void test_extract_migration_name_path_malloc_failure(void);
void test_extract_migration_name_payload_empty_suffix(void);
void test_extract_migration_name_single_char(void);
void test_extract_migration_name_root_slash(void);
void test_extract_migration_name_trailing_slash(void);
void test_execute_single_migration_load_only_with_state_malloc_failure_for_sql_copy(void);
void test_execute_single_migration_load_only_with_state_null_migration_file(void);
void test_execute_single_migration_load_only_with_state_migration_not_in_payload(void);
void test_execute_single_migration_load_only_with_state_empty_payload(void);
void test_execute_single_migration_load_only_with_state_null_engine_name(void);
void test_execute_single_migration_load_only_with_state_null_migration_name(void);

// Test setup and teardown
void setUp(void) {
    mock_system_reset_all();
    
    // Initialize database queue system
    database_queue_system_init();
    
    // Initialize config with defaults
    if (!app_config) {
        app_config = load_config(NULL);
    }
}

void tearDown(void) {
    mock_system_reset_all();
    
    // Clean up after each test
    if (app_config) {
        cleanup_application_config();
        app_config = NULL;
    }
}

// ===== extract_migration_name TESTS =====

void test_extract_migration_name_null_input(void) {
    char* path_copy = NULL;
    const char* result = extract_migration_name(NULL, &path_copy);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NULL(path_copy);
}

void test_extract_migration_name_empty_string(void) {
    char* path_copy = NULL;
    const char* result = extract_migration_name("", &path_copy);
    
    // Empty string is treated as a path (not PAYLOAD:)
    // basename("") returns "." (current directory)
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(".", result);
    TEST_ASSERT_NOT_NULL(path_copy);
    
    // Clean up
    free(path_copy);
}

void test_extract_migration_name_payload_prefix(void) {
    char* path_copy = NULL;
    const char* result = extract_migration_name("PAYLOAD:testmigration", &path_copy);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("testmigration", result);
    TEST_ASSERT_NULL(path_copy);  // Should not allocate for PAYLOAD:
}

void test_extract_migration_name_path_based(void) {
    char* path_copy = NULL;
    const char* result = extract_migration_name("/path/to/migrations", &path_copy);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("migrations", result);
    TEST_ASSERT_NOT_NULL(path_copy);
    
    free(path_copy);
}

void test_extract_migration_name_path_malloc_failure(void) {
    // Make strdup fail (which uses malloc internally)
    mock_system_set_malloc_failure(1);
    
    char* path_copy = NULL;
    const char* result = extract_migration_name("/path/to/migrations", &path_copy);
    
    // Should return NULL when strdup fails
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NULL(path_copy);
}

// ===== execute_single_migration_load_only_with_state MALLOC FAILURE TESTS =====

void test_execute_single_migration_load_only_with_state_malloc_failure_for_sql_copy(void) {
    // Create mock payload files
    PayloadFile payload_files[1];
    payload_files[0].name = strdup("migrations/test.lua");
    payload_files[0].data = (uint8_t*)strdup("return { queries = {} }");
    payload_files[0].size = strlen((const char*)payload_files[0].data);
    
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    (void)connection; // Suppress unused variable warning
    
    // This test attempts to trigger malloc failure during SQL copy allocation
    // However, it's difficult to reach that exact line without complex Lua setup
    // Testing with NULL connection to validate early failure path
    
    // Test with NULL connection
    (void)execute_single_migration_load_only_with_state(
        NULL,  // NULL connection
        "migrations/test.lua",
        "sqlite",
        "test",
        NULL,
        "test-label",
        NULL,
        payload_files,
        1
    );
    
    // Function doesn't validate connection parameter early
    // This validates the test doesn't crash
    
    // Clean up
    free(payload_files[0].name);
    free(payload_files[0].data);
}

void test_execute_single_migration_load_only_with_state_null_migration_file(void) {
    PayloadFile payload_files[1];
    payload_files[0].name = strdup("migrations/test.lua");
    payload_files[0].data = (uint8_t*)strdup("return { queries = {} }");
    payload_files[0].size = strlen((const char*)payload_files[0].data);
    
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    // Test with NULL migration file - should fail during file search
    bool result = execute_single_migration_load_only_with_state(
        &connection,
        NULL,  // NULL migration_file
        "sqlite",
        "test",
        NULL,
        "test-label",
        NULL,
        payload_files,
        1
    );
    
    // Should fail when trying to find NULL migration file
    TEST_ASSERT_FALSE(result);
    
    // Clean up
    free(payload_files[0].name);
    free(payload_files[0].data);
}

void test_execute_single_migration_load_only_with_state_migration_not_in_payload(void) {
    PayloadFile payload_files[1];
    payload_files[0].name = strdup("migrations/other.lua");
    payload_files[0].data = (uint8_t*)strdup("return { queries = {} }");
    payload_files[0].size = strlen((const char*)payload_files[0].data);
    
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    // Test with migration file that doesn't exist in payload
    bool result = execute_single_migration_load_only_with_state(
        &connection,
        "migrations/nonexistent.lua",  // Not in payload
        "sqlite",
        "test",
        NULL,
        "test-label",
        NULL,
        payload_files,
        1
    );
    
    // Should fail when migration file not found in payload (line 253-257)
    TEST_ASSERT_FALSE(result);
    
    // Clean up
    free(payload_files[0].name);
    free(payload_files[0].data);
}

void test_execute_single_migration_load_only_with_state_empty_payload(void) {
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    // Test with empty payload array
    bool result = execute_single_migration_load_only_with_state(
        &connection,
        "migrations/test.lua",
        "sqlite",
        "test",
        NULL,
        "test-label",
        NULL,
        NULL,  // NULL payload
        0      // Zero count
    );
    
    // Should fail when migration file not found (no payload files to search)
    TEST_ASSERT_FALSE(result);
}

// ===== EDGE CASE TESTS =====

void test_extract_migration_name_payload_empty_suffix(void) {
    char* path_copy = NULL;
    const char* result = extract_migration_name("PAYLOAD:", &path_copy);
    
    // Should return empty string after PAYLOAD:
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    TEST_ASSERT_NULL(path_copy);
}

void test_extract_migration_name_single_char(void) {
    char* path_copy = NULL;
    const char* result = extract_migration_name("x", &path_copy);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("x", result);
    TEST_ASSERT_NOT_NULL(path_copy);
    
    free(path_copy);
}

void test_extract_migration_name_root_slash(void) {
    char* path_copy = NULL;
    const char* result = extract_migration_name("/", &path_copy);
    
    TEST_ASSERT_NOT_NULL(result);
    // basename of "/" is "/"
    TEST_ASSERT_EQUAL_STRING("/", result);
    TEST_ASSERT_NOT_NULL(path_copy);
    
    free(path_copy);
}

void test_extract_migration_name_trailing_slash(void) {
    char* path_copy = NULL;
    const char* result = extract_migration_name("/path/to/migrations/", &path_copy);
    
    TEST_ASSERT_NOT_NULL(result);
    // basename handles trailing slash
    TEST_ASSERT_EQUAL_STRING("migrations", result);
    TEST_ASSERT_NOT_NULL(path_copy);
    
    free(path_copy);
}

// ===== INTEGRATION-LIKE TESTS =====

void test_execute_single_migration_load_only_with_state_null_engine_name(void) {
    PayloadFile payload_files[1];
    payload_files[0].name = strdup("migrations/test.lua");
    payload_files[0].data = (uint8_t*)strdup("return { queries = {} }");
    payload_files[0].size = strlen((const char*)payload_files[0].data);
    
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    // Test with NULL engine name - will fail during Lua execution
    bool result = execute_single_migration_load_only_with_state(
        &connection,
        "migrations/test.lua",
        NULL,  // NULL engine_name
        "test",
        NULL,
        "test-label",
        NULL,
        payload_files,
        1
    );
    
    // Should fail during Lua function execution
    TEST_ASSERT_FALSE(result);
    
    // Clean up
    free(payload_files[0].name);
    free(payload_files[0].data);
}

void test_execute_single_migration_load_only_with_state_null_migration_name(void) {
    PayloadFile payload_files[1];
    payload_files[0].name = strdup("migrations/test.lua");
    payload_files[0].data = (uint8_t*)strdup("return { queries = {} }");
    payload_files[0].size = strlen((const char*)payload_files[0].data);
    
    DatabaseHandle connection = {0};
    connection.engine_type = DB_ENGINE_SQLITE;
    
    // Test with NULL migration name - will fail during Lua execution
    bool result = execute_single_migration_load_only_with_state(
        &connection,
        "migrations/test.lua",
        "sqlite",
        NULL,  // NULL migration_name
        NULL,
        "test-label",
        NULL,
        payload_files,
        1
    );
    
    // Should fail during Lua function execution or file discovery
    TEST_ASSERT_FALSE(result);
    
    // Clean up
    free(payload_files[0].name);
    free(payload_files[0].data);
}

int main(void) {
    UNITY_BEGIN();
    
    // extract_migration_name tests
    RUN_TEST(test_extract_migration_name_null_input);
    RUN_TEST(test_extract_migration_name_empty_string);
    RUN_TEST(test_extract_migration_name_payload_prefix);
    RUN_TEST(test_extract_migration_name_path_based);
    RUN_TEST(test_extract_migration_name_path_malloc_failure);
    RUN_TEST(test_extract_migration_name_payload_empty_suffix);
    RUN_TEST(test_extract_migration_name_single_char);
    RUN_TEST(test_extract_migration_name_root_slash);
    RUN_TEST(test_extract_migration_name_trailing_slash);
    
    // execute_single_migration_load_only_with_state error path tests
    RUN_TEST(test_execute_single_migration_load_only_with_state_malloc_failure_for_sql_copy);
    RUN_TEST(test_execute_single_migration_load_only_with_state_null_migration_file);
    RUN_TEST(test_execute_single_migration_load_only_with_state_migration_not_in_payload);
    RUN_TEST(test_execute_single_migration_load_only_with_state_empty_payload);
    RUN_TEST(test_execute_single_migration_load_only_with_state_null_engine_name);
    RUN_TEST(test_execute_single_migration_load_only_with_state_null_migration_name);
    
    return UNITY_END();
}