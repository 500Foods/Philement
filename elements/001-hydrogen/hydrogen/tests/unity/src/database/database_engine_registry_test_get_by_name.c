/*
 * Unity Test File: database_engine_registry_test_get_by_name
 * This file contains unit tests for database_engine_get_by_name()
 * (defined in src/database/database_engine_registry.c)
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>

// Forward declarations for engine interfaces
DatabaseEngineInterface* sqlite_get_interface(void);

// Test function prototypes
void test_database_engine_get_by_name_basic(void);
void test_database_engine_get_by_name_null_name(void);
void test_database_engine_get_by_name_not_found(void);

void setUp(void) {
    // Initialize the database engine system
    database_engine_init();

    // If no engines were registered (because no app config), register SQLite for testing
    if (database_engine_get_by_name("sqlite") == NULL) {
        DatabaseEngineInterface* sqlite_engine = sqlite_get_interface();
        if (sqlite_engine) {
            database_engine_register(sqlite_engine);
        }
    }
}

void tearDown(void) {
    // No per-test fixtures to clean up
}

void test_database_engine_get_by_name_basic(void) {
    // Test getting the real SQLite engine by name (it's registered in setUp)
    DatabaseEngineInterface* found = database_engine_get_by_name("sqlite");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("sqlite", found->name);
}

void test_database_engine_get_by_name_null_name(void) {
    // Test with NULL name
    DatabaseEngineInterface* found = database_engine_get_by_name(NULL);
    TEST_ASSERT_NULL(found);
}

void test_database_engine_get_by_name_not_found(void) {
    // Test with non-existent name
    DatabaseEngineInterface* found = database_engine_get_by_name("nonexistent_engine");
    TEST_ASSERT_NULL(found);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_engine_get_by_name_basic);
    RUN_TEST(test_database_engine_get_by_name_null_name);
    RUN_TEST(test_database_engine_get_by_name_not_found);

    return UNITY_END();
}
