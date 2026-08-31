/*
 * Unity Test File: mdns_server_probe_test_mdns_server_next_instance_name.c
 * Tests mdns_server_next_instance_name DNS-SD "Name (N)" formatting
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>

void test_next_instance_attempt_2(void);
void test_next_instance_attempt_1_is_base(void);
void test_next_instance_null_base(void);
void test_next_instance_null_out(void);

void setUp(void)
{
}

void tearDown(void)
{
}

void test_next_instance_attempt_2(void)
{
    char out[64];

    mdns_server_next_instance_name("Friendly", 2, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("Friendly (2)", out);
}

void test_next_instance_attempt_1_is_base(void)
{
    char out[64];

    mdns_server_next_instance_name("Friendly", 1, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("Friendly", out);
}

void test_next_instance_null_base(void)
{
    char out[64];

    out[0] = 'x';
    mdns_server_next_instance_name(NULL, 2, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("", out);
}

void test_next_instance_null_out(void)
{
    mdns_server_next_instance_name("Friendly", 2, NULL, 8);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_next_instance_attempt_2);
    RUN_TEST(test_next_instance_attempt_1_is_base);
    RUN_TEST(test_next_instance_null_base);
    RUN_TEST(test_next_instance_null_out);
    return UNITY_END();
}
