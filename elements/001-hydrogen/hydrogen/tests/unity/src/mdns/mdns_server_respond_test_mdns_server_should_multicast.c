/*
 * Unity Test File: mdns_server_respond_test_mdns_server_should_multicast.c
 * Tests multicast dest: QM/QU yes, legacy no
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>

void test_should_multicast_qm(void);
void test_should_multicast_qu(void);
void test_should_multicast_legacy(void);

void setUp(void)
{
}

void tearDown(void)
{
}

void test_should_multicast_qm(void)
{
    TEST_ASSERT_EQUAL_INT(1, mdns_server_should_multicast(0, 0));
}

void test_should_multicast_qu(void)
{
    TEST_ASSERT_EQUAL_INT(1, mdns_server_should_multicast(0, 1));
}

void test_should_multicast_legacy(void)
{
    TEST_ASSERT_EQUAL_INT(0, mdns_server_should_multicast(1, 0));
    TEST_ASSERT_EQUAL_INT(0, mdns_server_should_multicast(1, 1));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_should_multicast_qm);
    RUN_TEST(test_should_multicast_qu);
    RUN_TEST(test_should_multicast_legacy);
    return UNITY_END();
}
