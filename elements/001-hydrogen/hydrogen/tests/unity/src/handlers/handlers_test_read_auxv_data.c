/*
 * Unity Test File: handlers_read_auxv_data
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/handlers/handlers.h>
#include <string.h>

void test_read_auxv_zero_max(void);
void test_read_auxv_self(void);

void setUp(void) {}
void tearDown(void) {}

void test_read_auxv_zero_max(void) {
    unsigned char buf[1];
    TEST_ASSERT_EQUAL_UINT(0, handlers_read_auxv_data(buf, 0));
}

void test_read_auxv_self(void) {
    unsigned char buf[8192];
    memset(buf, 0, sizeof(buf));
    size_t n = handlers_read_auxv_data(buf, sizeof(buf));
    TEST_ASSERT_TRUE(n > 0);
    TEST_ASSERT_TRUE(n <= sizeof(buf));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_read_auxv_zero_max);
    RUN_TEST(test_read_auxv_self);
    return UNITY_END();
}
