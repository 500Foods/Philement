/*
 * Unity Test File: mdns_server_probe_test_mdns_server_next_hostname.c
 * Tests mdns_server_next_hostname host-N.local rule
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>

void test_next_hostname_attempt_2(void);
void test_next_hostname_attempt_1(void);
void test_next_hostname_nulls(void);

void setUp(void)
{
}

void tearDown(void)
{
}

void test_next_hostname_attempt_2(void)
{
    char out[64];

    mdns_server_next_hostname("host", 2, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("host-2.local", out);
}

void test_next_hostname_attempt_1(void)
{
    char out[64];

    mdns_server_next_hostname("host", 1, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("host.local", out);
}

void test_next_hostname_nulls(void)
{
    char out[8];

    out[0] = 'x';
    mdns_server_next_hostname(NULL, 2, out, sizeof out);
    TEST_ASSERT_EQUAL_STRING("", out);
    mdns_server_next_hostname("host", 2, NULL, 8);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_next_hostname_attempt_2);
    RUN_TEST(test_next_hostname_attempt_1);
    RUN_TEST(test_next_hostname_nulls);
    return UNITY_END();
}
