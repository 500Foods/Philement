/*
 * Unity tests for normalize_engine_name.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/migration/migration.h>

void test_normalize_engine_name_known(void);
void test_normalize_engine_name_unknown(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_normalize_engine_name_known(void) {
    TEST_ASSERT_EQUAL_STRING("postgresql", normalize_engine_name("postgresql"));
    TEST_ASSERT_EQUAL_STRING("postgresql", normalize_engine_name("postgres"));
    TEST_ASSERT_EQUAL_STRING("mysql", normalize_engine_name("mysql"));
    TEST_ASSERT_EQUAL_STRING("sqlite", normalize_engine_name("sqlite"));
    TEST_ASSERT_EQUAL_STRING("db2", normalize_engine_name("db2"));
    TEST_ASSERT_EQUAL_STRING("firebird", normalize_engine_name("firebird"));
}

void test_normalize_engine_name_unknown(void) {
    TEST_ASSERT_NULL(normalize_engine_name(NULL));
    TEST_ASSERT_NULL(normalize_engine_name("cockroach"));
    TEST_ASSERT_NULL(normalize_engine_name("mssql"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_normalize_engine_name_known);
    RUN_TEST(test_normalize_engine_name_unknown);
    return UNITY_END();
}
