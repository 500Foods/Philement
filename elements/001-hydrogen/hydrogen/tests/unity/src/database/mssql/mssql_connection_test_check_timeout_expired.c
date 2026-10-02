/*
 * Unity Test File: MSSQL check_timeout_expired
 * Tests mssql_check_timeout_expired() utility function
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/mssql/connection.h>

void setUp(void) {
}

void tearDown(void) {
}

void test_mssql_check_timeout_expired_null_timeout(void);
void test_mssql_check_timeout_expired_future(void);
void test_mssql_check_timeout_expired_past(void);
void test_mssql_check_timeout_expired_exact(void);
void test_mssql_check_timeout_expired_negative_timeout(void);

void test_mssql_check_timeout_expired_null_timeout(void) {
    time_t start = time(NULL);
    bool result = mssql_check_timeout_expired(start, 0);
    TEST_ASSERT_TRUE(result);
}

void test_mssql_check_timeout_expired_future(void) {
    time_t start = time(NULL) - 60;
    bool result = mssql_check_timeout_expired(start, 120);
    TEST_ASSERT_FALSE(result);
}

void test_mssql_check_timeout_expired_past(void) {
    time_t start = time(NULL) - 120;
    bool result = mssql_check_timeout_expired(start, 60);
    TEST_ASSERT_TRUE(result);
}

void test_mssql_check_timeout_expired_exact(void) {
    time_t start = time(NULL) - 10;
    bool result = mssql_check_timeout_expired(start, 10);
    TEST_ASSERT_TRUE(result);
}

void test_mssql_check_timeout_expired_negative_timeout(void) {
    time_t start = time(NULL);
    bool result = mssql_check_timeout_expired(start, -1);
    TEST_ASSERT_TRUE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mssql_check_timeout_expired_null_timeout);
    RUN_TEST(test_mssql_check_timeout_expired_future);
    RUN_TEST(test_mssql_check_timeout_expired_past);
    RUN_TEST(test_mssql_check_timeout_expired_exact);
    RUN_TEST(test_mssql_check_timeout_expired_negative_timeout);

    return UNITY_END();
}
