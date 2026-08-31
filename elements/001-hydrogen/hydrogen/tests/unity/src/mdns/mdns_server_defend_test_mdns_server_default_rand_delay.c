/*
 * Unity Test File: mdns_server_defend_test_mdns_server_default_rand_delay.c
 * Tests the deterministic random delay function for shared-record timing.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>

void test_rand_delay_in_range(void);
void test_rand_delay_equal_bounds(void);
void test_rand_delay_min_zero(void);

void setUp(void)
{
}

void tearDown(void)
{
}

void test_rand_delay_in_range(void)
{
    uint32_t result;
    result = mdns_server_default_rand_delay(20, 120);
    TEST_ASSERT_TRUE(result >= 20);
    TEST_ASSERT_TRUE(result <= 120);
}

void test_rand_delay_equal_bounds(void)
{
    uint32_t result;
    result = mdns_server_default_rand_delay(50, 50);
    TEST_ASSERT_EQUAL_UINT32(50, result);
}

void test_rand_delay_min_zero(void)
{
    uint32_t result;
    result = mdns_server_default_rand_delay(0, 5);
    TEST_ASSERT_TRUE(result <= 5);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_rand_delay_in_range);
    RUN_TEST(test_rand_delay_equal_bounds);
    RUN_TEST(test_rand_delay_min_zero);
    return UNITY_END();
}
