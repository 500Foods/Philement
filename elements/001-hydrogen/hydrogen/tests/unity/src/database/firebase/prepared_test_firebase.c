/*
 * Unity tests for Phase 3 Firebase prepared-statement stubs.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebase/prepared.h>

void test_firebase_prepare_unprepare_stubs(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_prepare_unprepare_stubs(void) {
    TEST_ASSERT_FALSE(firebase_prepare_statement(NULL, "n", "sql", NULL, false));
    DatabaseHandle handle = {0};
    handle.engine_type = DB_ENGINE_FIREBASE;
    PreparedStatement* stmt = NULL;
    TEST_ASSERT_FALSE(firebase_prepare_statement(&handle, "n", "SELECT 1", &stmt, true));
    TEST_ASSERT_NULL(stmt);
    PreparedStatement existing = {0};
    TEST_ASSERT_FALSE(firebase_unprepare_statement(&handle, &existing));
    TEST_ASSERT_FALSE(firebase_unprepare_statement(NULL, &existing));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_prepare_unprepare_stubs);
    return UNITY_END();
}
