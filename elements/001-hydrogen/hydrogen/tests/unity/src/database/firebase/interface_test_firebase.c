/*
 * Unity tests for firebase_get_interface.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebase/interface.h>

void test_firebase_get_interface_not_null(void);
void test_firebase_get_interface_valid_structure(void);
void test_firebase_get_interface_function_pointers(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_get_interface_not_null(void) {
    DatabaseEngineInterface* interface = firebase_get_interface();
    TEST_ASSERT_NOT_NULL(interface);
}

void test_firebase_get_interface_valid_structure(void) {
    DatabaseEngineInterface* interface = firebase_get_interface();
    TEST_ASSERT_NOT_NULL(interface);
    TEST_ASSERT_EQUAL(DB_ENGINE_FIREBASE, interface->engine_type);
    TEST_ASSERT_NOT_NULL(interface->name);
    TEST_ASSERT_EQUAL_STRING("firebase", interface->name);
}

void test_firebase_get_interface_function_pointers(void) {
    DatabaseEngineInterface* interface = firebase_get_interface();
    TEST_ASSERT_NOT_NULL(interface);
    TEST_ASSERT_NOT_NULL(interface->connect);
    TEST_ASSERT_NOT_NULL(interface->disconnect);
    TEST_ASSERT_NOT_NULL(interface->health_check);
    TEST_ASSERT_NOT_NULL(interface->reset_connection);
    TEST_ASSERT_NOT_NULL(interface->execute_query);
    TEST_ASSERT_NOT_NULL(interface->execute_prepared);
    TEST_ASSERT_NOT_NULL(interface->begin_transaction);
    TEST_ASSERT_NOT_NULL(interface->commit_transaction);
    TEST_ASSERT_NOT_NULL(interface->rollback_transaction);
    TEST_ASSERT_NOT_NULL(interface->prepare_statement);
    TEST_ASSERT_NOT_NULL(interface->unprepare_statement);
    TEST_ASSERT_NOT_NULL(interface->get_connection_string);
    TEST_ASSERT_NOT_NULL(interface->validate_connection_string);
    TEST_ASSERT_NOT_NULL(interface->escape_string);
    TEST_ASSERT_NOT_NULL(interface->cancel_inflight);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_get_interface_not_null);
    RUN_TEST(test_firebase_get_interface_valid_structure);
    RUN_TEST(test_firebase_get_interface_function_pointers);
    return UNITY_END();
}
