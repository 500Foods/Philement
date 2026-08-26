/*
 * Unity Test File: land_mcp_subsystem
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/landing/landing.h>
#include <src/mcp/mcp.h>

int land_mcp_subsystem(void);

void test_land_mcp_subsystem_not_running(void);
void test_land_mcp_subsystem_after_init(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_land_mcp_subsystem_not_running(void) {
    int result = land_mcp_subsystem();
    TEST_ASSERT_EQUAL(1, result);
}

void test_land_mcp_subsystem_after_init(void) {
    mcp_init_state();
    TEST_ASSERT_TRUE(mcp_is_initialized());

    int result = land_mcp_subsystem();

    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_FALSE(mcp_is_initialized());
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_land_mcp_subsystem_not_running);
    RUN_TEST(test_land_mcp_subsystem_after_init);

    return UNITY_END();
}
