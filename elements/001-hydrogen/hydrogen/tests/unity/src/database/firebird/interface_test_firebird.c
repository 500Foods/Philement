/*
 * Unity Test File: Firebird Interface Functions
 * Tests firebird_get_interface and engine metadata functions.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebird/interface.h>

// Function prototypes for test functions
void test_firebird_get_interface_not_null(void);
void test_firebird_get_interface_valid_structure(void);
void test_firebird_get_interface_function_pointers(void);
void test_firebird_engine_get_version(void);
void test_firebird_engine_get_description(void);
void test_firebird_engine_is_available(void);

void setUp(void) {
    // Set up test fixtures
}

void tearDown(void) {
    // Clean up test fixtures
}

void test_firebird_get_interface_not_null(void) {
    DatabaseEngineInterface* interface = firebird_get_interface();
    TEST_ASSERT_NOT_NULL(interface);
}

void test_firebird_get_interface_valid_structure(void) {
    DatabaseEngineInterface* interface = firebird_get_interface();
    TEST_ASSERT_NOT_NULL(interface);
    TEST_ASSERT_EQUAL(DB_ENGINE_FIREBIRD, interface->engine_type);
    TEST_ASSERT_NOT_NULL(interface->name);
    TEST_ASSERT_EQUAL_STRING("firebird", interface->name);
}

void test_firebird_get_interface_function_pointers(void) {
    DatabaseEngineInterface* interface = firebird_get_interface();
    TEST_ASSERT_NOT_NULL(interface);

    TEST_ASSERT_NOT_NULL(interface->connect);
    TEST_ASSERT_NOT_NULL(interface->disconnect);
    TEST_ASSERT_NOT_NULL(interface->execute_query);
    TEST_ASSERT_NOT_NULL(interface->begin_transaction);
    TEST_ASSERT_NOT_NULL(interface->commit_transaction);
    TEST_ASSERT_NOT_NULL(interface->rollback_transaction);
    TEST_ASSERT_NOT_NULL(interface->get_connection_string);
    TEST_ASSERT_NOT_NULL(interface->validate_connection_string);
    TEST_ASSERT_NOT_NULL(interface->cancel_inflight);
}

void test_firebird_engine_get_version(void) {
    const char* version = firebird_engine_get_version();
    TEST_ASSERT_NOT_NULL(version);
    TEST_ASSERT_NOT_EQUAL(0, strlen(version));
}

void test_firebird_engine_get_description(void) {
    const char* desc = firebird_engine_get_description();
    TEST_ASSERT_NOT_NULL(desc);
    TEST_ASSERT_NOT_EQUAL(0, strlen(desc));
}

void test_firebird_engine_is_available(void) {
    bool available = firebird_engine_is_available();
    TEST_ASSERT_TRUE(available);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_firebird_get_interface_not_null);
    RUN_TEST(test_firebird_get_interface_valid_structure);
    RUN_TEST(test_firebird_get_interface_function_pointers);
    RUN_TEST(test_firebird_engine_get_version);
    RUN_TEST(test_firebird_engine_get_description);
    RUN_TEST(test_firebird_engine_is_available);

    return UNITY_END();
}
