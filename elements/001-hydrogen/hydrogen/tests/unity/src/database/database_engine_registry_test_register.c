/*
 * Unity Test File: database_engine_registry_test_register
 * This file contains unit tests for database_engine_register()
 * (defined in src/database/database_engine_registry.c).
 *
 * NOTE: the engine registry is process-global static state and there is no
 * public de-registration API. Unity runs every test in this file inside a
 * single process, so the tests are ordered to be consistent with that shared
 * state: a fresh engine type (DB_ENGINE_AI, which database_engine_init() never
 * auto-registers) is used so the register/already-registered sequence is
 * deterministic.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>

// A mock engine using DB_ENGINE_AI, which is never auto-registered by
// database_engine_init(), guaranteeing a clean slot for these tests.
static DatabaseEngineInterface mock_engine = {
    .engine_type = DB_ENGINE_AI,
    .name = (char*)"ai_test_engine",
    .connect = NULL,
    .disconnect = NULL,
    .health_check = NULL,
    .reset_connection = NULL,
    .execute_query = NULL,
    .execute_prepared = NULL,
    .begin_transaction = NULL,
    .commit_transaction = NULL,
    .rollback_transaction = NULL,
    .prepare_statement = NULL,
    .unprepare_statement = NULL,
    .get_connection_string = NULL,
    .validate_connection_string = NULL,
    .escape_string = NULL
};

// Test function prototypes
void test_database_engine_register_null_engine(void);
void test_database_engine_register_invalid_type(void);
void test_database_engine_register_basic(void);
void test_database_engine_register_already_registered(void);

void setUp(void) {
    // Ensure the engine system is initialized so registration is permitted.
    database_engine_init();
}

void tearDown(void) {
    // No per-test fixtures; registry state is intentionally shared (see header).
}

void test_database_engine_register_null_engine(void) {
    // Registering a NULL engine must fail and must not touch the registry.
    bool result = database_engine_register(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_database_engine_register_invalid_type(void) {
    // An engine with engine_type >= DB_ENGINE_MAX is rejected before the
    // registry is touched.
    DatabaseEngineInterface invalid_engine = mock_engine;
    invalid_engine.engine_type = DB_ENGINE_MAX;

    bool result = database_engine_register(&invalid_engine);
    TEST_ASSERT_FALSE(result);
}

void test_database_engine_register_basic(void) {
    // First registration of the DB_ENGINE_AI slot must succeed.
    bool result = database_engine_register(&mock_engine);
    TEST_ASSERT_TRUE(result);

    // And it must now be discoverable by name.
    DatabaseEngineInterface* found = database_engine_get_by_name("ai_test_engine");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_PTR(&mock_engine, found);
}

void test_database_engine_register_already_registered(void) {
    // The DB_ENGINE_AI slot was filled by test_database_engine_register_basic.
    // A second registration for the same type must fail.
    bool result = database_engine_register(&mock_engine);
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    // Ordering matters: state-free rejections first, then the register/
    // already-registered pair that depends on the shared registry slot.
    RUN_TEST(test_database_engine_register_null_engine);
    RUN_TEST(test_database_engine_register_invalid_type);
    RUN_TEST(test_database_engine_register_basic);
    RUN_TEST(test_database_engine_register_already_registered);

    return UNITY_END();
}
