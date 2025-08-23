/*
 * Unity Test File: Launch Plan Tests
 * This file contains unit tests for launch_plan.c functionality
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Forward declaration of the function we want to test
bool handle_launch_plan(const ReadinessResults* results);
void test_handle_launch_plan_null_parameter(void);
void test_handle_launch_plan_no_subsystems_ready(void);
void test_handle_launch_plan_all_subsystems_ready(void);
void test_handle_launch_plan_some_subsystems_ready(void);
void test_handle_launch_plan_single_subsystem_ready(void);
void test_handle_launch_plan_single_subsystem_not_ready(void);
void test_handle_launch_plan_empty_results(void);
void test_handle_launch_plan_maximum_subsystems(void);
void test_handle_launch_plan_inconsistent_counters_any_ready_false(void);
void test_handle_launch_plan_zero_total_checked_but_any_ready_true(void);
void test_handle_launch_plan_all_standard_subsystems(void);
void test_handle_launch_plan_mixed_subsystem_names(void);
void test_handle_launch_plan_boundary_conditions(void);
void test_handle_launch_plan_boundary_conditions_not_ready(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

void test_handle_launch_plan_null_parameter(void) {
    // Test that handle_launch_plan returns false when passed NULL
    bool result = handle_launch_plan(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_handle_launch_plan_no_subsystems_ready(void) {
    // Test when no subsystems are ready (any_ready = false)
    ReadinessResults results = {
        .results = {
            {"Registry", false},
            {"Payload", false},
            {"Threads", false}
        },
        .total_checked = 3,
        .total_ready = 0,
        .total_not_ready = 3,
        .any_ready = false
    };
    
    bool result = handle_launch_plan(&results);
    TEST_ASSERT_FALSE(result);
}

void test_handle_launch_plan_all_subsystems_ready(void) {
    // Test when all subsystems are ready
    ReadinessResults results = {
        .results = {
            {"Registry", true},
            {"Payload", true},
            {"Threads", true}
        },
        .total_checked = 3,
        .total_ready = 3,
        .total_not_ready = 0,
        .any_ready = true
    };
    
    bool result = handle_launch_plan(&results);
    TEST_ASSERT_TRUE(result);
}

void test_handle_launch_plan_some_subsystems_ready(void) {
    // Test when some subsystems are ready, some are not
    ReadinessResults results = {
        .results = {
            {"Registry", true},
            {"Payload", false},
            {"Threads", true},
            {"Network", false},
            {"Database", true}
        },
        .total_checked = 5,
        .total_ready = 3,
        .total_not_ready = 2,
        .any_ready = true
    };
    
    bool result = handle_launch_plan(&results);
    TEST_ASSERT_TRUE(result);
}

void test_handle_launch_plan_single_subsystem_ready(void) {
    // Test when only one subsystem is ready
    ReadinessResults results = {
        .results = {
            {"Registry", true},
            {"Payload", false},
            {"Threads", false}
        },
        .total_checked = 3,
        .total_ready = 1,
        .total_not_ready = 2,
        .any_ready = true
    };
    
    bool result = handle_launch_plan(&results);
    TEST_ASSERT_TRUE(result);
}

void test_handle_launch_plan_single_subsystem_not_ready(void) {
    // Test when only one subsystem is not ready
    ReadinessResults results = {
        .results = {
            {"Registry", true},
            {"Payload", true},
            {"Threads", false}
        },
        .total_checked = 3,
        .total_ready = 2,
        .total_not_ready = 1,
        .any_ready = true
    };
    
    bool result = handle_launch_plan(&results);
    TEST_ASSERT_TRUE(result);
}

void test_handle_launch_plan_empty_results(void) {
    // Test with empty results (no subsystems checked)
    ReadinessResults results = {
        .results = {{0}},
        .total_checked = 0,
        .total_ready = 0,
        .total_not_ready = 0,
        .any_ready = false
    };
    
    bool result = handle_launch_plan(&results);
    TEST_ASSERT_FALSE(result);
}

void test_handle_launch_plan_maximum_subsystems(void) {
    // Test with maximum number of subsystems (15 as per state_types.h)
    ReadinessResults results = {
        .results = {
            {"Registry", true},
            {"Payload", true},
            {"Threads", false},
            {"Network", true},
            {"Database", false},
            {"Logging", true},
            {"WebServer", true},
            {"API", false},
            {"Swagger", true},
            {"WebSocket", false},
            {"Terminal", true},
            {"mDNS Server", true},
            {"mDNS Client", false},
            {"MailRelay", true},
            {"Print", false}
        },
        .total_checked = 15,
        .total_ready = 9,
        .total_not_ready = 6,
        .any_ready = true
    };
    
    bool result = handle_launch_plan(&results);
    TEST_ASSERT_TRUE(result);
}

void test_handle_launch_plan_inconsistent_counters_any_ready_false(void) {
    // Test edge case where any_ready is false but counters might suggest otherwise
    // This tests the function's reliance on any_ready field specifically
    ReadinessResults results = {
        .results = {
            {"Registry", true},
            {"Payload", true}
        },
        .total_checked = 2,
        .total_ready = 2,
        .total_not_ready = 0,
        .any_ready = false  // This is the key test - any_ready overrides other indicators
    };
    
    bool result = handle_launch_plan(&results);
    TEST_ASSERT_FALSE(result);
}

void test_handle_launch_plan_zero_total_checked_but_any_ready_true(void) {
    // Test edge case where total_checked is 0 but any_ready is true
    ReadinessResults results = {
        .results = {{0}},
        .total_checked = 0,
        .total_ready = 0,
        .total_not_ready = 0,
        .any_ready = true
    };
    
    bool result = handle_launch_plan(&results);
    TEST_ASSERT_TRUE(result);
}

void test_handle_launch_plan_all_standard_subsystems(void) {
    // Test with all standard subsystems in the expected order
    ReadinessResults results = {
        .results = {
            {"Registry", true},        // 1
            {"Payload", true},         // 2
            {"Threads", true},         // 3
            {"Network", true},         // 4
            {"Database", true},        // 5
            {"Logging", true},         // 6
            {"WebServer", true},       // 7
            {"API", true},             // 8
            {"Swagger", true},         // 9
            {"WebSocket", true},       // 10
            {"Terminal", true},        // 11
            {"mDNS Server", true},     // 12
            {"mDNS Client", true},     // 13
            {"MailRelay", true},       // 14
            {"Print", true}            // 15
        },
        .total_checked = 15,
        .total_ready = 15,
        .total_not_ready = 0,
        .any_ready = true
    };
    
    bool result = handle_launch_plan(&results);
    TEST_ASSERT_TRUE(result);
}

void test_handle_launch_plan_mixed_subsystem_names(void) {
    // Test with various subsystem names to ensure string handling works correctly
    ReadinessResults results = {
        .results = {
            {"", true},                    // Empty string
            {"A", false},                  // Single character
            {"VeryLongSubsystemName", true}, // Long name
            {"Name-With-Dashes", false},   // Special characters
            {"Name_With_Underscores", true} // Underscores
        },
        .total_checked = 5,
        .total_ready = 3,
        .total_not_ready = 2,
        .any_ready = true
    };
    
    bool result = handle_launch_plan(&results);
    TEST_ASSERT_TRUE(result);
}

void test_handle_launch_plan_boundary_conditions(void) {
    // Test boundary condition: exactly one subsystem, ready
    ReadinessResults results = {
        .results = {
            {"OnlySubsystem", true}
        },
        .total_checked = 1,
        .total_ready = 1,
        .total_not_ready = 0,
        .any_ready = true
    };
    
    bool result = handle_launch_plan(&results);
    TEST_ASSERT_TRUE(result);
}

void test_handle_launch_plan_boundary_conditions_not_ready(void) {
    // Test boundary condition: exactly one subsystem, not ready
    ReadinessResults results = {
        .results = {
            {"OnlySubsystem", false}
        },
        .total_checked = 1,
        .total_ready = 0,
        .total_not_ready = 1,
        .any_ready = false
    };
    
    bool result = handle_launch_plan(&results);
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();
    
    // Basic functionality tests
    RUN_TEST(test_handle_launch_plan_null_parameter);
    RUN_TEST(test_handle_launch_plan_no_subsystems_ready);
    RUN_TEST(test_handle_launch_plan_all_subsystems_ready);
    RUN_TEST(test_handle_launch_plan_some_subsystems_ready);
    
    // Single subsystem tests
    RUN_TEST(test_handle_launch_plan_single_subsystem_ready);
    RUN_TEST(test_handle_launch_plan_single_subsystem_not_ready);
    
    // Edge cases
    RUN_TEST(test_handle_launch_plan_empty_results);
    RUN_TEST(test_handle_launch_plan_maximum_subsystems);
    RUN_TEST(test_handle_launch_plan_inconsistent_counters_any_ready_false);
    RUN_TEST(test_handle_launch_plan_zero_total_checked_but_any_ready_true);
    
    // Comprehensive tests
    RUN_TEST(test_handle_launch_plan_all_standard_subsystems);
    RUN_TEST(test_handle_launch_plan_mixed_subsystem_names);
    
    // Boundary conditions
    RUN_TEST(test_handle_launch_plan_boundary_conditions);
    RUN_TEST(test_handle_launch_plan_boundary_conditions_not_ready);
    
    return UNITY_END();
}
